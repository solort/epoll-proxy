#include "./ev_epoll.h"
#include "./protocol.h"
#include "./fd.h"
#include "./config.hpp"


int epoll_fd[MAX_THREADS];

int listen_fd; 
struct fdtab *fdtab = nullptr;


static void proxy_init()
{
    listen_fd = front_tcp_init(FRONTEND_PORT, FRONTEND_MAX_CONNECTION); 

    epoll_fd[0] = epoll_init();
    printf("create epoll, fd: [%d]\n", epoll_fd[0]);
    epoll_register(EPOLLIN | EPOLLERR | EPOLLHUP, epoll_fd[0], listen_fd);
    printf("register listen fd, in epoll_fd[%d]\n", epoll_fd[0]);

    fdtab = (struct fdtab *)calloc(MAX_CONNECTION, sizeof(struct fdtab));
    printf("proxy init done\n");
}

int run_loop()
{
    int count;
    int status;
    struct epoll_event events[MAX_EVENTS];
    
    status = epoll_wait(epoll_fd[0], events, MAX_EVENTS, -1);
    if (status < 0)
    {
        printf("epoll status < 0 \n");
        exit(1);
    }
    

    for (count = 0; count < status; ++count)
    {
        auto fd = (int)events[count].data.fd;
        auto ev = (uint32_t)events[count].events;

        if ((ev & EPOLLERR) || (ev & EPOLLHUP)) 
        {
            if (fd == listen_fd)
            {
                printf("EPOLLERR:fd == listen_fd\n");
                exit(1);
            }
            else
            {
                printf("\nsession connection break, fd: [%d], pair: [%d], events: %o\n", fd, fdtab[fd].pair_fd, ev);
                if (fdtab[fd].flag == 1)
                {
                    resetfd(fdtab[fd].pair_fd);
                }
                resetfd(fd);
            }
        }
        else if (ev & EPOLLRDHUP)
        {
            // client closed
            printf("\nsession connection break, fd: [%d], pair: [%d], events: %u\n", fd, fdtab[fd].pair_fd, ev);
            resetfd(fdtab[fd].pair_fd);
            resetfd(fd);
        }
        else if (fd == listen_fd)
        {
            if (ev & EPOLLIN)
            {
                int server_fd = 0;
                int client_fd = tcp_accept(epoll_fd[0], fd); 
                if (client_fd == -1)
                    continue;
                printf("\nfront connect established, client fd: [%d]\n", client_fd);
                
                if (fdtab[client_fd].flag != 1)
                {
                    fdtab[client_fd].owner = FD_EV_CLIENT;

                    int conflag = 0;
                    if((server_fd = back_tcp_init(BACKEND_IP_ADDR_1, conflag)) > 0)
                    {
                        fdtab[client_fd].pair_fd = server_fd;
                        fdtab[server_fd].pair_fd = client_fd;
                        fdtab[server_fd].owner = FD_EV_SERVER;
                        fdtab[client_fd].flag = 1;
                        fdtab[server_fd].flag = conflag;
                    }
                    else
                    {
                        printf("back_tcp_init error, fd: [%d]\n", client_fd);
                        continue;
                    }
                }
                else
                {
                    printf("fd[%d] used but also want to register\n", client_fd);
                    continue;
                }
            }
            else
            {
                printf("unknown event & fd == listen_fd\n");
            }
        }
        else if (ev & EPOLLOUT)
        {
            if(fdtab[fd].flag)
            {
                int loop_count = 0;
                while(1)
                {
                    auto sendret = tcp_sendret(fd);
                    if (sendret < 0)
                    {
                        printf("error, sendret: %ld\n",sendret);
                        break;
                    }
                    else if (sendret == 0)       
                    {
                        // clear out
                        fdtab[fd].events = fdtab[fd].events & (~EPOLLOUT);
                        epoll_changemod(fdtab[fd].events, epoll_fd[0], fd);
                        int pair_fd = fdtab[fd].pair_fd;
                        fdtab[pair_fd].events = fdtab[pair_fd].events | EPOLLIN;
                        epoll_changemod(fdtab[pair_fd].events, epoll_fd[0], pair_fd);
                        break;
                    }
                    else
                    {
                        ++loop_count;
                        if (loop_count > 2)
                        {
                            // printf("loop_count > 2,fd: %d\n",fd);
                            break;
                        }
                        
                    }
                }
            }
            else
            {
                int err = -1;
                socklen_t len = sizeof(int);
                
                if (getsockopt(events[count].data.fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
                {
                    resetfd(events[count].data.fd);
                    printf("getsockopt errno:%d %s\n", errno, strerror(errno));
                    continue;
                }
                
                if (err != 0)
                {
                    printf("not access\n");
                    continue;
                }

                epoll_changemod(EPOLLIN | EPOLLERR | EPOLLHUP, epoll_fd[0], fd);
                fdtab[fd].events = EPOLLIN;
                epoll_changemod(EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP, epoll_fd[0], fdtab[fd].pair_fd);
                fdtab[fdtab[fd].pair_fd].events = EPOLLIN;

                fdtab[fd].flag = 1;
                printf("back connect established,client fd: %d\n", fd);
            }
            
        }
        else if (ev & EPOLLIN)
        {
            int pfd = fdtab[fd].pair_fd;
            SocketBuffer* pstBuffer = get_socket_buffer(pfd);
	        // printf("[%d] socket buffer head %d tail %d\n", pfd, pstBuffer->mHead, pstBuffer->mTail);
            if (pstBuffer->mTail != pstBuffer->mHead) {
                printf("[%d] skip recieve due to pair buffer nonempty\n", pfd);
                continue;
            }
            if ((fdtab[fd].flag == 1)&&(fdtab[pfd].flag == 1))
            {
                auto ret = tcp_receive(fd, pstBuffer->mBuffer, FUNC_BUFFER_SIZE);
                if (ret < 0)
                {
                    // printf("Error occurred!\n");
                    continue;
                }
                else if(ret == 0)
                {
                    printf("ret == 0\n");
                }
                else
                {
                    pstBuffer->mHead = 0;
                    pstBuffer->mTail = ret;
                }
                
                //printf("[%d] receive and send len[%d]\n", fd, ret);
                tcp_send(fdtab[fd].pair_fd, pstBuffer, ret);
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    printf("\n-------------   proxy init   -------------\n");
    proxy_init();

    printf("\n-------------    run loop    -------------\n");
    while (1)
    {
        run_loop();
    }

    printf("cant reach\n");
    close(epoll_fd[0]);
    free(fdtab);
    fdtab = nullptr;
    close(listen_fd);

    return 0;
}
