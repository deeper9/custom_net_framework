#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace sylar
{

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id { 0 };
static std::atomic<uint64_t> s_fiber_count { 0 };

// 当前执行协程
static thread_local Fiber* t_fiber = nullptr;
// 主协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

// 协程分配栈大小
static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024 * 1, "fiber stack size");

// 内存分配器
class MallocStackAllocator {
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

// private，只能在内部创建，创建主协程是使用的默认构造函数
Fiber::Fiber()
{
    m_state = EXEC;
    SetThis(this);

    // 获取当前线程上下文，当前为主协程上下应为空 --todo
    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    ++s_fiber_count;
}

// 创建子协程调用的构造函数
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id)
    , m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    if (!use_caller)
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    else 
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
}

Fiber::~Fiber()
{
    --s_fiber_count;
    // 主协程没有栈
    if (m_stack) {
        SYLAR_ASSERT(m_state == TERM            // 执行结束未来得及释放
            || m_state == EXCEPT                // 异常终止未释放内存
            || m_state == INIT);                // 刚创建
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    else {
        SYLAR_ASSERT(!m_cb); // 主协程没有cb（构造函数决定）
        SYLAR_ASSERT(m_state == EXEC);  //主协程主动析构时一定是exec状态

        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
    SYLAR_LOG_DEBUG(g_logger) << "~Fiber id=" << m_id;
}

// 利用已执行完的协程留下的内存重新利用执行栈
void Fiber::reset(std::function<void()> cb)
{
    SYLAR_ASSERT(m_stack);          // 子协程有栈，才有可能有cb
    SYLAR_ASSERT(m_state == TERM
        || m_state == EXCEPT
        || m_state == INIT);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::swapIn()
{
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;

    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)){
        SYLAR_ASSERT2(false, "swapcontext");
    } 
}
void Fiber::swapOut()
{
    // 当前协程切换到后台，一切子协程都由主协程调度，所以切换当前执行协程为主协程
    SetThis(Scheduler::GetMainFiber());
    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }

}

void Fiber::call()
{
    SetThis(this);
    m_state = EXEC;
    SYLAR_LOG_ERROR(g_logger) << getId();
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back()
{
    SetThis(t_threadFiber.get());
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

// 设置当前执行的协程
void Fiber::SetThis(Fiber* f)
{
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis()
{
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    // 创建主协程
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;

    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady()
{
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers()
{
    return s_fiber_count;
}

// 协程回调函数
void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch(std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except";
    }
    // 执行结束后协程结束不会主动切换到主协程
    // Fiber::ptr cur = GetThis();给当前协程引用计数+1，导致不会被析构
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();
    // swapout后直接切换到主协程，不会执行后续
    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc()
{
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch(std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except";
    }
    // 执行结束后协程结束不会主动切换到主协程
    // Fiber::ptr cur = GetThis();给当前协程引用计数+1，导致不会被析构
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    // swapout后直接切换到主协程，不会执行后续
    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

uint64_t Fiber::GetFiberId()
{
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

}
