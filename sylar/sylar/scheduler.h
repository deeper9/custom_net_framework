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
    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否使用当前调用线程
     * @param[in] name 协程调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const;
    /**
     * @brief 返回当前协程调度器
     */
    static Scheduler* GetThis();
    /**
     * @brief 返回当前协程调度器的调度协程
     */
    static Fiber* GetMainFiber();
    // 启动调度器，创建线程池
    void start();
    void stop();

     /**
     * @brief 调度协程
     * @param[in] fc 协程或函数
     * @param[in] thread 协程执行的线程id,-1标识任意线程
     */
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

    /**
     * @brief 批量调度协程
     * @param[in] begin 协程数组的开始
     * @param[in] end 协程数组的结束
     */
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
    /**
     * @brief 通知协程调度器有任务了
     */
    virtual void tickle();
    /**
     * @brief 返回是否可以停止
     */
    virtual bool stopping();
    /**
     * @brief 协程无任务可调度时执行idle协程
     */
    virtual void idle();
    void setThis();
     /**
     * @brief 协程调度函数
     */
    void run();

    bool hasIdleThreads() { return m_idleThreadCount > 0; }
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
    /**
     * @brief 协程/函数/线程组
     */
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread; // 线程id

        FiberAndThread(Fiber::ptr f, int thr)
            : fiber(f)
            , thread(thr)
        {
            
        }

        // 传两层指针为了swap减少智能指针引用
        FiberAndThread(Fiber::ptr* f, int thr)
            : thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr)
            : cb(f), thread(thr) {
            
        }

        FiberAndThread(std::function<void()>* f, int thr)
            : thread(thr) {
            cb.swap(*f);
        }

        // 在stl中的对象一定要有默认构造函数，否则构造的对象无法初始化
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
    // 线程池
    std::vector<Thread::ptr> m_threads;
    // 待执行的协程队列
    std::list<FiberAndThread> m_fibers;
    // use_caller为true有效，调度协程
    Fiber::ptr m_rootFiber;
    std::string m_name;
protected:
    // 协程下的线程id数组
    std::vector<int> m_threadIds;
    // 线程数量
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = { 0 };
    std::atomic<size_t> m_idleThreadCount = { 0 };
    bool m_stopping = true;
    bool m_autoStop = false;
    // 主线程id
    int m_rootThread = 0;
};
}

#endif