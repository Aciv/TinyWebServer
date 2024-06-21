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

class cWebServer{
private:
    bool InitSocket();
    int SetFdNonblock(int a_Fd);

    void AddClient_(int a_Fd, sockaddr_in a_Addr);
    void DealListen();
    void DealWrite(int* a_Client);
    void DealRead(int* a_Client);
    void CloseConn(int* a_Client);

    void OnRead(int* a_Client);
    void OnWrite(int* a_Client);
    void OnProcess(int* a_Client);
    
    static const int m_MAXFD = 65536;

    int m_Port;
    int m_TimeoutMs;  /* ∫¡√ÎMS */
    bool m_IsClose;
    int m_ListenFd;
    
    uint32_t m_ListenEvent;
    uint32_t m_ConnEvent;
    std::unique_ptr<cEpoller> m_Epoller;
    std::unordered_map<int, int> m_Users;
public:
    cWebServer(int a_Port, int a_TimeoutMs);
    ~cWebServer();
    void Start();
};

#endif