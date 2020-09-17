#include "./fd.h"

extern struct fdtab *fdtab;
extern int epoll_fd[MAX_THREADS];

void resetfd(int fd)
{
    close(fd);
    printf("close fd: [%d]\n",fd);
    
    fdtab[fd].pair_fd = 0;
    fdtab[fd].owner = 0;
    fdtab[fd].flag = 0;
    free(fdtab[fd].fdbuf);
    fdtab[fd].fdbuf = NULL;
}

SocketBuffer* get_socket_buffer(int fd)
{
    if(fdtab[fd].fdbuf == NULL)
    {
        fdtab[fd].fdbuf = (struct SocketBuffer *)malloc(sizeof(struct SocketBuffer));
        fdtab[fd].fdbuf->mHead = fdtab[fd].fdbuf->mTail = 0;
    }
    return fdtab[fd].fdbuf;
}
