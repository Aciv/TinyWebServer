#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>    

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "httpRequest.h"
#include "httpResponse.h"

class cHttpConn{
public:
    cHttpConn();
    ~cHttpConn();

    void init(int a_Fd,const sockaddr_in &a_Addr);
    void Close();

    ssize_t Read(int * a_Error);
    ssize_t Write(int * a_Error);


    int ToWriteBytes() { 
        return m_Iov[0].iov_len + m_Iov[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return m_Request.IsKeepAlive();
    }
 


    bool Process();

    int GetFd();
    int GetPort() const;
    const char* GetIP() const;
    sockaddr_in GetAddr() const;

    static bool m_IsET;
    static const char* m_SrcDir;
    static std::atomic<int> m_UserCount;

private:
    int m_Fd;
    struct  sockaddr_in m_Addr;

    bool m_IsClose;


    int m_IovCnt;
    struct iovec m_Iov[2];
    
    cBuffer m_ReadBuff; 
    cBuffer m_WriteBuff; 

    cHttpRequest m_Request;
    cHttpResponse m_Response;
};

#endif  // HTTPCONN_H