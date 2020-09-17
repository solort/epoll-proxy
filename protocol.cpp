#include "./protocol.h"

extern struct fdtab *fdtab;
extern int epoll_fd[MAX_THREADS];


int front_tcp_init(int port, int max_connection)
{
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return -1;
    }
    int on = 1;
    ioctl(fd, FIONBIO, &on);

    struct sockaddr_in server_sockaddr;
    bzero(&server_sockaddr, sizeof(server_sockaddr));
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = inet_addr(LOCAL_IP_ADDR);
    server_sockaddr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr)) == -1)
    {
        perror("bind error");
        return -1;
    }

    if (listen(fd, max_connection) < 0)
    {
        perror("listen error\n");
        return -1;
    }

    return fd;
}

int back_tcp_init(const char *ip_addr, int &conflag)
{
    int connect_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr, &serveraddr.sin_addr.s_addr);
    serveraddr.sin_port = htons(BACNEND_PORT);
    int on = 1;
    ioctl(connect_fd, FIONBIO, &on);

    if (connect(connect_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        if (errno == EINPROGRESS)
        {
            epoll_register(EPOLLOUT | EPOLLERR | EPOLLHUP, epoll_fd[0], connect_fd);
            fdtab[connect_fd].events = EPOLLOUT | EPOLLERR | EPOLLHUP;
            printf("EINPROGRESS! connection fd: [%d]\n", connect_fd);
            return connect_fd;
        }
        close(connect_fd);
        printf("connect error, fd: [%d]\n", connect_fd);
        return -1;
    }

    conflag = 1;
    epoll_register(EPOLLIN | EPOLLERR | EPOLLHUP, epoll_fd[0], connect_fd);
    fdtab[connect_fd].events = EPOLLIN | EPOLLERR | EPOLLHUP;

    printf("back connect established,client fd: %d\n", connect_fd);
    return connect_fd;
}

int tcp_accept(int epoll_fd, int fd)
{
    int connect_fd = accept(fd, NULL, NULL);

    if (connect_fd < 0)
    {
        printf("accept error\n");
        return -1;
    }
    
    int on = 1;
    ioctl(connect_fd, FIONBIO, &on);

    epoll_register(EPOLLERR | EPOLLHUP | EPOLLRDHUP, epoll_fd, connect_fd);
    fdtab[connect_fd].events = EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    return connect_fd;
}

int64_t tcp_receive(int fd, void *buf, int64_t n)
{
    auto ret = recv(fd, buf, n, 0);
    if (ret < 0)
    {
        if (errno == ECONNRESET)
        {
            perror("ECONNRESET");
        }
        else if (errno == EAGAIN)
        {
            return -2;
        }
        else
        {
            perror("tcp_receive error");
        }
        printf("recv error close fd is %d\n", fd);

        if (fdtab[fd].flag == 1)
        {
            resetfd(fdtab[fd].pair_fd);
            resetfd(fd);
        }
        else
        {
            printf("fd[%d] not use but recv still\n", fd);
            exit(1);
        }

        return -1;
    }
    else if (ret == 0)
    {
        if (fdtab[fd].flag == 1)
        {
            printf("\nsession connection break, fd: [%d], pair: [%d], events: tcp_receive\n", fd, fdtab[fd].pair_fd);
            resetfd(fdtab[fd].pair_fd);
            resetfd(fd);
        }
        else
        {
            printf("fd not use but recv still\n");
            exit(1);
        }
        return -1;
    }

    return ret;
}

int64_t tcp_send(int fd, SocketBuffer* sendBuffer, int64_t remainded)
{ 
    int64_t sended = 0;
    char* pszTmp = &sendBuffer->mBuffer[sendBuffer->mHead]; 
    int64_t again_count = 0;
    while(remainded > 0 && again_count < 2)
    {
        sended = send(fd, pszTmp, (size_t)remainded, 0);
        if (sended > 0)
        {
            // printf("[%d] sendret len[%d]\n", fd, sended);
            sendBuffer->mHead += sended;
            pszTmp += sended;
            remainded -= sended;
        }
        else if (errno == EAGAIN)
        {
            ++again_count;
            if (again_count > 1)
            {
                int ret;
                fdtab[fd].events |= EPOLLOUT;
                if ((ret = epoll_changemod(fdtab[fd].events, epoll_fd[0], fd)) < 0)
                {
                    printf("[%d] add epoll_out failed %d\n", fd, ret);
                }
                else
                {
                    // remove IN from receive side
                    int pair_fd = fdtab[fd].pair_fd;
                    fdtab[pair_fd].events = fdtab[pair_fd].events & (~EPOLLIN);
                    if ((ret = epoll_changemod(fdtab[pair_fd].events, epoll_fd[0], pair_fd)) < 0)
                    {
                         printf("[%d] remove epoll_in failed %d\n", fd, ret);
                    }
                }
                break;
            }
        }
        else
        {
            if (fdtab[fd].flag == 1)
            {
                printf("\nsession connection break, fd: [%d], pair: [%d], events: tcp_send\n", fd, fdtab[fd].pair_fd);
                resetfd(fdtab[fd].pair_fd);
                resetfd(fd);
                return -1;
            }
            else
            {
                printf("fd[%d] not use but send still \n", fd);
                exit(1);
            }
        }
    }
    if (remainded == 0) {
        sendBuffer->mTail = sendBuffer->mHead = 0;
    }
    return remainded;
}

int64_t tcp_sendret(int fd)
{
    SocketBuffer* sendBuffer = get_socket_buffer(fd);
    if (NULL == sendBuffer)
    {
        return -1;
    }
 
    int64_t remainded = sendBuffer->mTail - sendBuffer->mHead;
    int64_t sended = 0;
    char* pszTmp = &sendBuffer->mBuffer[sendBuffer->mHead]; 
    int64_t again_count = 0;
    while(remainded > 0 && again_count < 2)
    {
        sended = send(fd, pszTmp, (size_t)remainded, 0);
        if (sended > 0)
        {
            // printf("[%d] sendret len[%d]\n", fd, sended);
            sendBuffer->mHead += sended;
            pszTmp += sended;
            remainded -= sended;
        }
        else if (errno == EAGAIN)
        {
            ++ again_count;
            continue;
        }
        else
        {
            printf("\nsession connection break, fd: [%d], pair: [%d], events: tcp_sendret\n", fd, fdtab[fd].pair_fd);
            resetfd(fdtab[fd].pair_fd);
            resetfd(fd);
            break;
        }
    }
    if (remainded == 0) {
        sendBuffer->mTail = sendBuffer->mHead = 0;
    }
    return remainded;
}
