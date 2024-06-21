#include "webServer.h"


bool cWebServer::InitSocket() {
    int ret;
    struct sockaddr_in addr;
    if(m_Port > 65535 || m_Port < 1024) {
        return false;
    }
    addr.sin_family = AF_INET;  // protocal
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_Port);



    m_ListenFd = socket(AF_INET, SOCK_STREAM, 0);

    if(m_ListenFd < 0) {
        return false;
    }

    ret = setsockopt(m_ListenFd, SOL_SOCKET, SO_LINGER, nullptr, 0);  //端口复用
    if(ret < 0) {
        close(m_ListenFd);
        return false;
    }

    int optval = 1;

    ret = setsockopt(m_ListenFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        close(m_ListenFd);
        return false;
    }

    ret = bind(m_ListenFd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        close(m_ListenFd);
        return false;
    }

    ret = listen(m_ListenFd, 6);  // 被动监听 第二个参数为监听队列
    if(ret < 0) {
        close(m_ListenFd);
        return false;
    }
    ret = m_Epoller->AddFd(m_ListenFd,  m_ListenEvent | EPOLLIN);
    if(ret == 0) {
        close(m_ListenFd);
        return false;
    }
    SetFdNonblock(m_ListenFd);
    return true;
}

cWebServer::cWebServer(int a_Port, int a_TimeoutMs)
    : m_Port(a_Port), m_TimeoutMs(a_TimeoutMs),
      m_ListenEvent(EPOLLRDHUP | EPOLLET),
      m_ConnEvent(EPOLLONESHOT | EPOLLRDHUP | EPOLLET),
      m_Epoller(new cEpoller) {

    if(!InitSocket()) m_IsClose = true;
}

cWebServer::~cWebServer() {
    close(m_ListenFd);
    m_IsClose = true;

}



void cWebServer::Start(){
    int TimeMs = 1;  

    while(!m_IsClose) {
        int EventCnt = m_Epoller->Wait(TimeMs);
        for(int i = 0; i < EventCnt; i++) {
            int fd = m_Epoller->GetEventFd(i);
            uint32_t Events = m_Epoller->GetEvents(i);
            if(fd == m_ListenFd) {
                DealListen();
            }
            else if(Events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(m_Users.count(fd) > 0);
                CloseConn(&m_Users[fd]);
            }
            else if(Events & EPOLLIN) {
                assert(m_Users.count(fd) > 0);
                DealRead(&m_Users[fd]);
            }
            else if(Events & EPOLLOUT) {
                assert(m_Users.count(fd) > 0);
                DealWrite(&m_Users[fd]);
            }
        }
    }
}



int cWebServer::SetFdNonblock(int a_Fd) {
    assert(a_Fd > 0);
    return fcntl(a_Fd, F_SETFL, fcntl(a_Fd, F_GETFD, 0) | O_NONBLOCK);
}