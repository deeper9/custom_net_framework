#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include <memory>
#include "mutex.h"
#include "iomanager.h"
#include "singleton.h"

namespace sylar
{
/**
 * @brief 文件句柄上下文类
 * @details 管理文件句柄类型(是否socket)
 *          是否阻塞,是否关闭,读/写超时时间
 */
class FdCtx : public std::enable_shared_from_this<FdCtx>
{
public:
    typedef std::shared_ptr<FdCtx> ptr;
    FdCtx(int fd);
    ~FdCtx();

    bool init();
    bool isInit() const { return m_isInit; }
    bool isSocket() const { return m_isSocket; }
     /**
     * @brief 是否已关闭
     */
    bool isClose() const { return m_isClosed; }
    bool close();
    /**
     * @brief 获取是否用户主动设置的非阻塞
     */
    void setUserNonblock(bool v) { m_userNonblock = v; }
    bool getUserNonblock() const { return m_userNonblock; }
    /**
     * @brief 设置系统非阻塞
     * @param[in] v 是否阻塞
     */
    void setSysNonblock(bool v) { m_sysNonblock = v; }   
    bool getSysNonblock() const { return m_sysNonblock; }
     /**
     * @brief 设置超时时间
     * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * @param[in] v 时间毫秒
     */
    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);

private:
    bool m_isInit = true;
    bool m_isSocket = true;
    bool m_sysNonblock = true;
    bool m_userNonblock = true;
    bool m_isClosed = true;
    int m_fd;

    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;

    sylar::IOManager* m_iomanager;
};

class FdManager 
{
public:
    typedef RWMutex RWMutexType;
    FdManager();

    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;

}

#endif