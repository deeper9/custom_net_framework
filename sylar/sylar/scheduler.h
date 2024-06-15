#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include "thread.h"
#include "fiber.h"
#include "mutex.h"

namespace sylar
{
class Scheduler
{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType; 

    // user_caller=true表示将创建协程调度器构造函数的线程纳入调度器管理
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const;

    static Scheduler* GetThis();
    // 协程调度器的主协程
    static Fiber* GetMainFiber();

    void start();
    void stop();

    template <class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = schedulerNoLock(fc, thread);
        }
        if (need_tickle) {
            tickle();
        }
    }

    template <class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = schedulerNoLock(&*begin) || need_tickle;
            }
            ++begin;
        }
        if (need_tickle) {
            tickle();
        }
    }

protected:
    virtual void tickle();
    virtual bool stopping();
    virtual void idle(); // 没有任务的时候执行
    void setThis();
    void run();
private:
    template <class FiberOrCb>
    bool schedulerNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr)
            : fiber(f)
            , thread(thr)
        {
            
        }

        // 传两层指针为了swap减少智能指针引用
        FiberAndThread(Fiber::ptr* f, int thr)
            : thread(thr)
        {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr)
            : cb(f), thread(thr)
        {
            
        }

        FiberAndThread(std::function<void()>* f, int thr)
            : thread(thr)
        {
            cb.swap(*f);
        }

        // 在stl中一定要有默认构造函数，否则构造的对象无法初始化
        FiberAndThread()
            : thread(-1)
        {}

        void reset()
        {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

    
private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;
    // 执行scheduler的协程，
    Fiber::ptr m_rootFiber;
    std::string m_name;
protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = { 0 };
    std::atomic<size_t> m_idleThreadCount = { 0 };
    bool m_stopping = true;
    bool m_autoStop = false;
    int m_rootThread = 0;
};
}

#endif