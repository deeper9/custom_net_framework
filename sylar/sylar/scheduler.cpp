#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace sylar
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
// thread_local:每个线程都有各自的独立实例
static thread_local Scheduler* t_scheduler = nullptr;
// 调度器的主协程
static thread_local Fiber* t_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    : m_name(name)
{
    SYLAR_ASSERT(threads > 0);

    if (use_caller) {
        sylar::Fiber::GetThis(); // 创建线程的主线程
        --threads;  // 当前线程纳入调度器后，不需要再重新创建线程

        SYLAR_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        sylar::Thread::SetName(m_name);

        t_fiber = m_rootFiber.get();
        m_rootThread = sylar::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        // 当前线程不归调度器管理
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler()
{
    SYLAR_ASSERT(m_stopping);
    // 可能存在多个协程调度器
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

const std::string& Scheduler::getName() const
{
    return m_name;
}

Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber()
{
    return t_fiber;
}

void Scheduler::start()
{
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {
        return ;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());
    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), 
            m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();
}

// 两种情况：
// 1.使用use_caller: 一定要在创建scheduler的线程里面执行stop
// 2.不使用use_caller: 在任意线程可执行stop
void Scheduler::stop()
{
    SYLAR_LOG_DEBUG(g_logger) << "sc stop";
    m_autoStop = true;
    if (m_rootFiber && 
        m_threadCount == 0 &&
        ((m_rootFiber->getState() == Fiber::TERM || 
            m_rootFiber->getState() == Fiber::INIT)))
    {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if (stopping()) {
            return ;
        }
    }

    if (m_rootThread != -1) {
        SYLAR_ASSERT(GetThis() == this);
    } else {
        SYLAR_ASSERT(GetThis() != this);
    }
    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle(); // 唤醒线程执行，才会结束
    }
    // 主线程tickle，所有线程结束
    if (m_rootFiber) {
        SYLAR_LOG_DEBUG(g_logger) << "sc sto2";
        tickle();
    }
    if (m_rootFiber) {
        if (!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for (auto& i : thrs) {
        SYLAR_LOG_DEBUG(g_logger) << "sc stop1";
        i->join();
    }
}

void Scheduler::setThis()
{
    t_scheduler = this;
}

void Scheduler::run()
{
    SYLAR_LOG_INFO(g_logger) << "sc run";
    setThis();
    if (sylar::GetThreadId() != m_rootThread) {
        t_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while (true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        // 取出要执行的任务
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            // 拿出当前线程的任务，拿不到则tickle通知其他线程
            while (it != m_fibers.end()) {
                // 每个协程任务指定了对应线程，任务到达消息队列时释放信号
                // 获取到这个信号的线程不一定是该任务的指定的线程
                // tickle_me置为true，需要唤醒对应线程执行该任务
                //当前的执行的线程id与任务的线程id不同，tickle对应线程来执行
                if (it->thread != -1 && it->thread != sylar::GetThreadId()) { 
                    ++it;
                    tickle_me = true;
                    SYLAR_LOG_DEBUG(g_logger) << "tickle other thread";
                    continue;
                }
                SYLAR_ASSERT(it->fiber || it->cb);
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }
                ft = *it;
                tickle_me = true;   // 任务队列还有剩余，通知其他线程来进行调度
                m_fibers.erase(it);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }

        if (tickle_me) {
            SYLAR_LOG_DEBUG(g_logger) << "Scheduler run tickle";
            tickle();
        }
        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM
            ||  ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;
            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM
                && ft.fiber->getState() != Fiber::EXCEPT) {
                    ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if (ft.cb) {
            if (cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
                ft.cb = nullptr;
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::EXCEPT
                || cb_fiber->getState() == Fiber::TERM) {
                    cb_fiber->reset(nullptr);
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            } 
        } else {
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM
                && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            } 
        }
    }
}

void Scheduler::tickle()
{
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping()
{
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle()
{
    SYLAR_LOG_INFO(g_logger) << "idle";
    while (!stopping()) {
        sylar::Fiber::YieldToHold();
    }
}

}