#pragma once
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "./config.hpp"
#include "./fd.h"
#include "./ev_epoll.h"


int front_tcp_init(int port, int max_connection);
int tcp_accept(int epoll_fd, int fd);
int back_tcp_init(const char *ip_addr, int &conflag);
int64_t tcp_receive(int client_fd, void *buf, int64_t n);
int64_t tcp_send(int fd, SocketBuffer *pstBuffer, int64_t remainded);
int64_t tcp_sendret(int fd);