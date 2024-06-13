#ifndef __SYLAR_LOCK_H__
#define __SYLAR_LOCK_H__

#include <semaphore.h>
#include <atomic>
#include <pthread.h>

namespace sylar
{
class Semaphore
{
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();

private:
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

private:
    sem_t m_semaphore;
};

template<class T>
struct ScopedLockImpl
{
public:
    ScopedLockImpl(T& mutex)
        : m_mutex(mutex)
    {
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLockImpl()
    {
        m_mutex.unlock();
    } 

    void lock() 
    {
        if (!m_locked)
        {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock()
    {
        if (m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct ReadScopedLockImpl
{
public:
    ReadScopedLockImpl(T& mutex)
        : m_mutex(mutex)
    {
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl(){
        m_mutex.unlock();
    } 

    void lock() {
        if (!m_locked)
        {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock()
    {
        if (m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct WriteScopedLockImpl
{
public:
    WriteScopedLockImpl(T& mutex)
        : m_mutex(mutex)
    {
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl(){
        m_mutex.unlock();
    } 

    void lock() {
        if (!m_locked)
        {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock()
    {
        if (m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

class Spinlock
{
public:
    typedef ScopedLockImpl<Spinlock> Lock;
    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~Spinlock() 
    {
        pthread_spin_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_spin_lock(&m_mutex);
    }
    void unlock()
    {
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

class CASLock
{
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() {
        m_mutex.clear();
    }

    ~CASLock() {

    }

    void lock() {
        while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;
};

}

#endif