#pragma once

#include <sys/epoll.h>
#include <stdlib.h>
#include <cstdlib>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <errno.h>
#include "./config.hpp"
#include "./fd.h"

int epoll_init(void);
int epoll_register(int events_, int epoll_fd_, int fd_);
int epoll_changemod(int events_, int epoll_fd_, int fd_);