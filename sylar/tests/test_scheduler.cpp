#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "test in fiber";
    static int s_count = 5;
    sleep(1);
    if (--s_count >= 0) {
        sylar::Scheduler::GetThis()->schedule(&test_fiber, sylar::GetThreadId());
    }
}

int main() {
    SYLAR_LOG_INFO(g_logger) << "main";
    sylar::Scheduler sc(3, true, "test");
    sc.schedule(&test_fiber);
    sc.start();
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "over";
    return 0;
}