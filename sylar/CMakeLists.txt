cmake_minimum_required(VERSION 2.8) #最小版本
project(sylar) #项目名

#set(CMAKE_VERBOSE_MAKEFILE ON) #make过程中打印详细命令
# 编译参数
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

include_directories(
    .
    ..
    /home/lzhj/include
)

link_directories(/home/lzhj/lib)

# 源码路径
set(LIB_SRC
    sylar/log.cpp
    sylar/util.cpp
    sylar/config.cpp
    sylar/thread.cpp
    sylar/fiber.cpp
    sylar/lock.cpp
    sylar/mutex.cpp
    sylar/scheduler.cpp
)
# 生成库
add_library(sylar SHARED ${LIB_SRC})

set(LIB_LIB
    sylar
    pthread
    yaml-cpp
)

# 生成测试文件
add_executable(test tests/test.cpp)
add_dependencies(test sylar)
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config sylar)
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util sylar)
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber sylar)
target_link_libraries(test_fiber ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler sylar)
target_link_libraries(test_scheduler ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/LIB)