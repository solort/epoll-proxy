#pragma once

#define MAX_EVENTS 500              // epoll 设置
#define MAX_CONNECTION 1000         //最大连接数
#define FRONTEND_MAX_CONNECTION 500 // 前端 listen 设置
#define BACNEND_MAX_CONNECTION 500  // 后端 listen 设置

#define FUNC_BUFFER_SIZE 8192

#define LOCAL_IP_ADDR "9.134.233.89"
#define BACKEND_IP_ADDR_1 "9.32.175.177" 
#define BACKEND_IP_ADDR_2 "9.134.126.249"

#define FRONTEND_PORT 50000  // 监听端口
#define BACNEND_PORT 6667 // 后端服务器端口

#define MAX_THREADS 1
