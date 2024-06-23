#include "webServer.h"
#include <iostream>

bool cWebServer::InitSocket() {
    int ret;
    struct sockaddr_in addr;
    if(m_Port > 65535 || m_Port < 1024) {
        LOG_ERROR("Port:%d error!",  m_Port);
        return false;
    }

    addr.sin_family = AF_INET;  // protocal
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_Port);



    m_ListenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_ListenFd < 0) {
        LOG_ERROR("Create socket error!", m_Port);
        return false;
    }

    int optval = 1;
    ret = setsockopt(m_ListenFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(m_ListenFd);
        return false;
    }


    ret = bind(m_ListenFd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        close(m_ListenFd);
        LOG_ERROR("Bind Port:%d error!", m_Port);
        return false;
    }

    ret = listen(m_ListenFd, 6);  // 被动监听 第二个参数为监听队列
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", m_Port);
        close(m_ListenFd);
        return false;
    }
    ret = m_Epoller->AddFd(m_ListenFd,  m_ListenEvent | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(m_ListenFd);
        return false;
    }

    LOG_INFO("Server port:%d", m_Port);
    SetFdNonblock(m_ListenFd);
    return true;
}

cWebServer::cWebServer(int a_Port, int a_TimeoutMs, int a_TrigMod, 
                    int a_ThreadNum, bool a_openLog, int a_logLevel, int a_logQueSize)
    : m_Port(a_Port), m_TimeoutMs(a_TimeoutMs), m_ThreadNum(a_ThreadNum),
      m_ListenEvent(EPOLLRDHUP | EPOLLET),
      m_ConnEvent(EPOLLONESHOT | EPOLLRDHUP | EPOLLET),
      m_Epoller(new cEpoller()), m_ThreadPool(new cThreadPool(m_ThreadNum)) {

    m_SrcDir = getcwd(nullptr, 256);
    assert(m_SrcDir);
    
    strncat(m_SrcDir, "/resources/", 16);
    cHttpConn::m_UserCount = 0;
    cHttpConn::m_SrcDir = m_SrcDir;
    
    InitEventMode(a_TrigMod);
    if(!InitSocket()) m_IsClose = true;

    if(a_openLog) {
        Log::Instance()->init(a_logLevel, "./log", ".log", a_logQueSize);
        if(m_IsClose) { LOG_ERROR("========== Server init error!=========="); }
        else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d", m_Port);
            LOG_INFO("LogSys level: %d", a_logLevel);
            LOG_INFO("srcDir: %s", cHttpConn::m_SrcDir);
            LOG_INFO("ThreadPool num: %d", m_ThreadNum);
        }
    }
}

cWebServer::~cWebServer() {
    close(m_ListenFd);
    m_IsClose = true;

}

void cWebServer::InitEventMode(int a_TrigMode) {
    m_ListenEvent = EPOLLRDHUP;
    m_ConnEvent = EPOLLONESHOT | EPOLLRDHUP;
    switch (a_TrigMode)
    {
    case 0:
        break;
    case 1:
        m_ConnEvent |= EPOLLET;
        break;
    case 2:
        m_ListenEvent |= EPOLLET;
        break;
    case 3:
        m_ListenEvent |= EPOLLET;
        m_ConnEvent |= EPOLLET;
        break;
    default:
        m_ListenEvent |= EPOLLET;
        m_ConnEvent |= EPOLLET;
        break;
    }
    cHttpConn::m_IsET = (m_ConnEvent & EPOLLET);
}

void cWebServer::Start(){
    int TimeMs = 100;  

    if(!m_IsClose) { LOG_INFO("========== Server start ==========");  }
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
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void cWebServer::SendError(int a_fd, const char* a_info) {
    assert(a_fd > 0);
    int ret = send(a_fd, a_info, strlen(a_info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", a_fd);
    }
    close(a_fd);
}

void cWebServer::DealListen(){
     struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(m_ListenFd, (struct sockaddr *)&addr, &len);

        if(fd <= 0) { return;}

        else if(cHttpConn::m_UserCount >= m_MAXFD) {
            LOG_WARN("Clients is full!");
            return;
        }

        AddClient(fd, addr);
    } while(m_ListenEvent & EPOLLET);
}

void cWebServer::AddClient(int a_Fd, sockaddr_in a_Addr){
    assert(a_Fd > 0);
    m_Users[a_Fd].init(a_Fd, a_Addr);

    m_Epoller->AddFd(a_Fd, EPOLLIN | m_ConnEvent);
    SetFdNonblock(a_Fd);
    LOG_INFO("Client[%d] in!", m_Users[a_Fd].GetFd());
}

void cWebServer::DealRead(cHttpConn* a_Client){
    assert(a_Client);
    m_ThreadPool->AddTask(std::bind(&cWebServer::OnRead, this, a_Client));
}

void cWebServer::DealWrite(cHttpConn* a_Client){
    assert(a_Client);
    m_ThreadPool->AddTask(std::bind(&cWebServer::OnWrite, this, a_Client));
}

void cWebServer::OnRead(cHttpConn* a_Client){

    assert(a_Client);
    int ret = -1;
    int readErrno = 0;
    ret = a_Client->Read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn(a_Client);
        return;
    }
    
    OnProcess(a_Client);
}

// process if have deal read if done then put response message
void cWebServer::OnProcess(cHttpConn* a_Client) {
    if(a_Client->Process()) {
        m_Epoller->ModFd(a_Client->GetFd(), m_ConnEvent | EPOLLOUT);
    } else {
        m_Epoller->ModFd(a_Client->GetFd(), m_ConnEvent | EPOLLIN);
    }
}

void cWebServer::OnWrite(cHttpConn* a_Client){
    assert(a_Client);
    int ret = -1;
    int WriteErrno = 0;
    ret = a_Client->Write(&WriteErrno);

    if(a_Client->ToWriteBytes() == 0) {
        if(a_Client->IsKeepAlive()) {
            OnProcess(a_Client);
            return;
        }
    }

    else if(ret < 0) {
        if(WriteErrno == EAGAIN) {
            m_Epoller->ModFd(a_Client->GetFd(), m_ConnEvent | EPOLLOUT);
            return;
        }
    }
    CloseConn(a_Client);
}

void cWebServer::CloseConn(cHttpConn* a_Client){
    assert(a_Client);
    LOG_INFO("Client[%d] quit!", a_Client->GetFd());
    m_Epoller->DelFd(a_Client->GetFd());
    a_Client->Close();
}

int cWebServer::SetFdNonblock(int a_Fd) {
    assert(a_Fd > 0);
    return fcntl(a_Fd, F_SETFL, fcntl(a_Fd, F_GETFD, 0) | O_NONBLOCK);
}