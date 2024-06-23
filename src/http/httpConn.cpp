#include "httpConn.h"
#include <iostream>

const char* cHttpConn::m_SrcDir;
std::atomic<int> cHttpConn::m_UserCount;
bool cHttpConn::m_IsET;

cHttpConn::cHttpConn() : m_Fd(0),  m_IsClose(true) {
    m_Addr = {0};
}

cHttpConn::~cHttpConn() {
    Close();
}

void cHttpConn::Close() {
    m_Response.UnMapFile();

    if(m_IsClose == false){
        m_IsClose = true; 
        m_UserCount--;
        close(m_Fd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", m_Fd, GetIP(), GetPort(), (int)m_UserCount);
    }

}

int cHttpConn::GetFd(){
    return m_Fd;
}
int cHttpConn::GetPort() const {
    return m_Addr.sin_port;
}
const char *cHttpConn::GetIP() const {
    return inet_ntoa(m_Addr.sin_addr);
}
sockaddr_in cHttpConn::GetAddr() const {
    return m_Addr;
}

void cHttpConn::init(int a_Fd, const sockaddr_in& a_Addr) {
    assert(a_Fd > 0);
    m_UserCount++;
    m_Addr = a_Addr;
    m_Fd = a_Fd;
    m_WriteBuff.RetrieveAll();
    m_ReadBuff.RetrieveAll();
    m_IsClose = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", m_Fd, GetIP(), GetPort(), (int)m_UserCount);
}

ssize_t cHttpConn::Read(int* a_Error) {
    ssize_t len = -1;
    do {
        len = m_ReadBuff.ReadFd(m_Fd, a_Error);

        if (len <= 0) {
            break;
        }
    } while (m_IsET);
    return len;
}

ssize_t cHttpConn::Write(int* a_Error) {
    ssize_t len = -1;
    do {
        len = writev(m_Fd, m_Iov, m_IovCnt);
        if(len <= 0) {
            *a_Error = errno;
            break;
        }
        if(m_Iov[0].iov_len + m_Iov[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > m_Iov[0].iov_len) {
            m_Iov[1].iov_base = (uint8_t*) m_Iov[1].iov_base + (len - m_Iov[0].iov_len);
            m_Iov[1].iov_len -= (len - m_Iov[0].iov_len);
            if(m_Iov[0].iov_len) {
                m_WriteBuff.RetrieveAll();
                m_Iov[0].iov_len = 0;
            }
        }

        else {
            m_Iov[0].iov_base = (uint8_t*)m_Iov[0].iov_base + len; 
            m_Iov[0].iov_len -= len; 
            m_WriteBuff.Retrieve(len);
        }
    } while(m_IsET || ToWriteBytes() > 10240);
    return len;
}

bool cHttpConn::Process() {
    m_Request.Init();

    if(m_ReadBuff.ReadableBytes() <= 0) {
        return false;
    }

    else if(m_Request.Parse(m_ReadBuff)) {
        LOG_DEBUG("%s", m_Request.Path().c_str());
        m_Response.Init(m_SrcDir, m_Request.Path(), m_Request.IsKeepAlive(), 200);

    } else {
        m_Response.Init(m_SrcDir, m_Request.Path(), false, 400);
    }

    m_Response.MakeResponse(m_WriteBuff);
    /* 响应头 */
    m_Iov[0].iov_base = const_cast<char*>(m_WriteBuff.Peek());
    m_Iov[0].iov_len = m_WriteBuff.ReadableBytes();
    m_IovCnt = 1;

    /* 文件 */
    if(m_Response.FileLen() > 0  && m_Response.File()) {
        m_Iov[1].iov_base = m_Response.File();
        m_Iov[1].iov_len = m_Response.FileLen();
        m_IovCnt = 2;
    }

    
    LOG_DEBUG("filesize:%d, %d  to %d", m_Response.FileLen() , m_IovCnt, ToWriteBytes());
    return true;
}
