#pragma once
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <stdio.h>

#include "./config.hpp"

#define FD_EV_CLIENT 0
#define FD_EV_SERVER 1

struct SocketBuffer
{
    int64_t mHead;
    int64_t mTail;
    char     mBuffer[FUNC_BUFFER_SIZE];
};

struct fdtab
{
    int pair_fd;
    int owner;
    int flag;
    int events;
    struct SocketBuffer *fdbuf;
};


void resetfd(int fd);
SocketBuffer* get_socket_buffer(int fd);