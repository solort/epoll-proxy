#include <stdint.h>

#include "./ev_epoll.h"

extern int listen_fd;
extern struct fdtab *fdtab;

int epoll_init(void)
{
    int epoll_fd;
    if ((epoll_fd = epoll_create(MAX_EVENTS)) < 0)
    {
        perror("epoll create failed\n");
        return 1;
    }
    return epoll_fd;
}

int epoll_register(int events_, int epoll_fd_, int fd_)
{
    struct epoll_event event;
    event.data.fd = fd_;
    event.events = events_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd_, &event) != 0)
    {
        printf("epoll_register failed:%d, %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int epoll_changemod(int events_, int epoll_fd_, int fd_)
{
    struct epoll_event event;
    event.data.fd = fd_;
    event.events = events_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd_, &event) != 0)
    {
        printf("epoll_changemod failed:%d, %s\n", errno,strerror(errno));
        return -1;
    }
    return 0;
}
