#include "epoller.h"

cEpoller::cEpoller(size_t a_MaxEvent)
    : m_EpollFd(epoll_create(512)), m_EpollEvents(a_MaxEvent){
    assert(m_EpollFd >= 0 && m_EpollEvents.size() > 0);
}
cEpoller::~cEpoller(){
    close(m_EpollFd);
}

bool cEpoller::AddFd(int a_Fd, uint32_t a_Events){
    if(a_Fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = a_Fd;
    ev.events = a_Events;
    return 0 == epoll_ctl(m_EpollFd, EPOLL_CTL_ADD, a_Fd, &ev);
}

bool cEpoller::ModFd(int a_Fd, uint32_t a_Events){
    if(a_Fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = a_Fd;
    ev.events = a_Events;
    return 0 == epoll_ctl(m_EpollFd, EPOLL_CTL_MOD, a_Fd, &ev);
}
bool cEpoller::DelFd(int a_Fd){
    if(a_Fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(m_EpollFd, EPOLL_CTL_DEL, a_Fd, &ev);
}

int cEpoller::Wait(int a_TimeoutMs){
    return epoll_wait(m_EpollFd, &m_EpollEvents[0], static_cast<int>(m_EpollEvents.size()), a_TimeoutMs);
}

int cEpoller::GetEventFd(size_t a_i) const {
    assert(a_i < m_EpollEvents.size() && a_i >= 0);
    return m_EpollEvents[a_i].data.fd;
}

uint32_t cEpoller::GetEvents(size_t a_i) const {
    assert(a_i < m_EpollEvents.size() && a_i >= 0);
    return m_EpollEvents[a_i].events;
}