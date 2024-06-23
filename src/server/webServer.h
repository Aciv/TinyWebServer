#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <memory>
#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../pool/threadPool.h"
#include "../http/httpConn.h"
#include "../log/log.h"

class cWebServer{
private:

    void InitEventMode(int a_TrigMode);
    bool InitSocket();
    int SetFdNonblock(int a_Fd);

    void AddClient(int a_Fd, sockaddr_in a_Addr);
    void DealListen();
    void DealWrite(cHttpConn* a_Client);
    void DealRead(cHttpConn* a_Client);
    void CloseConn(cHttpConn* a_Client);

    void OnRead(cHttpConn* a_Client);
    void OnWrite(cHttpConn* a_Client);
    void OnProcess(cHttpConn* a_Client);

    void SendError(int a_fd, const char* a_info);

    static const int m_MAXFD = 65536;

    int m_Port;
    int m_TimeoutMs;  
    bool m_IsClose;
    int m_ListenFd;
    char* m_SrcDir;
    int m_ThreadNum;

    uint32_t m_ListenEvent;
    uint32_t m_ConnEvent;
    std::unique_ptr<cEpoller> m_Epoller;
    std::unordered_map<int, cHttpConn> m_Users;
    std::unique_ptr<cThreadPool> m_ThreadPool;
public:
    cWebServer(int a_Port, int a_TrigMod, int a_TimeoutMs, int a_ThreadNum, 
            bool a_openLog, int a_logLevel, int a_logQueSize);

    ~cWebServer();
    void Start();
};

#endif  // WEBSERVER_H