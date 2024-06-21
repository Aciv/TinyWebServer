#ifndef EPOLLER_H
#define EPOLLER_H
#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>


class cEpoller{
public:
    cEpoller(size_t a_MaxEvent = 1024);
    ~cEpoller();

    bool AddFd(int a_Fd, uint32_t a_Events);
    bool ModFd(int a_Fd, uint32_t a_Events);
    bool DelFd(int a_Fd);

    int Wait(int a_TimeoutMs = -1);

    int GetEventFd(size_t a_i) const;
    uint32_t GetEvents(size_t a_i) const;

private:
    int m_EpollFd;

    std::vector<struct epoll_event> m_EpollEvents;
};


#endif