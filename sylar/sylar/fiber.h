#ifndef __SYLAY_FIBER_H__
#define __SYLAY_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace sylar
{

class Fiber : public std::enable_shared_from_this<Fiber>
{
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,       // 协程刚被创建，未开始执行
        HOLD,       // 正在等待某些条件满足或外部唤醒，此状态下协程不执行，通常是等待io或其他异步事件
        EXEC,       // 协程正在执行或者即将执行
        TERM,       // 执行完成并终止
        READY,      // 准备就绪，通常表示等待被分配cpu时间
        EXCEPT    // 异常结束
    };
    Fiber(std::function<void()> cb, size_t stacksize = 0);
    ~Fiber();

    // 重置协程函数并重置状态，INIT,TERM
    void reset(std::function<void()> cb);
    // 切换到当前协程执行
    void swapIn(); 
    // 切换到后台
    void swapOut();

    uint64_t getId() const { return m_id; }

public:
    // 设置当前协程
    static void SetThis(Fiber* f);
    // 返回当前执行的协程
    static Fiber::ptr GetThis();
    // 协程切换到后台并设置为ready状态
    static void YieldToReady();
    // 协程切换到后台并设置为hold状态
    static void YieldToHold(); 
    // 总协程数
    static uint64_t TotalFibers(); 
    // 
    static void MainFunc();
    static uint64_t GetFiberId();

private:
    Fiber();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;
    ucontext_t m_ctx;
    void* m_stack = nullptr; // 栈的内存
    std::function<void()> m_cb;
};

}

#endif