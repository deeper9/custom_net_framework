#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include "lock.h"

namespace sylar
{

class Mutex
{
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex();
    ~Mutex();

    void lock();
    void unlock();
private:
    pthread_mutex_t m_mutex;
};

class NullMutex 
{
public:
    typedef  ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};

class NullRWMutex
{
public:
    typedef ReadScopedLockImpl<NullMutex> ReadLock;
    typedef WriteScopedLockImpl<NullMutex> WriteLock;
    NullRWMutex() {}
    ~NullRWMutex() {}
    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};

class RWMutex
{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;
    
    RWMutex();
    ~RWMutex();
    
    void rdlock();
    void wrlock();
    void unlock();
private:
    pthread_rwlock_t m_lock;
};

}

#endif