#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int volatile count = 0;
sylar::RWMutex s_mutex;
void func1()
{
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                           << " this.name: " << sylar::Thread::GetThis()->getName()
                           << " id: " << sylar::GetThreadId()
                           << " this.id" << sylar::Thread::GetThis()->getId();
    // 不加锁：多个线程可能读到同一个结果，导致结果异常
    // 加读锁：只保证多个线程读取正常，写操作不保证，所以多个线程可能读取相同的值
    // 写锁：一个线程读取数据后其他线程无法读写
    for (int i = 0; i < 1000000; ++i)
    {
        sylar::RWMutex::WriteLock lock(s_mutex);
        ++count;
    }
}

void func2()
{

}

int main()
{
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i)
    {
        sylar::Thread::ptr thr(new sylar::Thread(&func1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for (int i = 0; i < 5; ++i)
    {
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count=" << count;

    return 1;
}