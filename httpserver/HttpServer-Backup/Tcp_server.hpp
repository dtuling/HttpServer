#pragma once
#include <iostream>
#include <string>
#include <pthread.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "log.hpp"
// 单例模式的TcpServer
class TcpServer
{
private:
    const static int backlog = 20;
    TcpServer(uint16_t port) : _port(port), _listen_sock(-1) {}

    TcpServer(const TcpServer &st) = delete;

public:
    static TcpServer *GetInstance(int port)
    {
        // 静态互斥锁使用宏初始化  不用destory了
        static pthread_mutex_t mutx = PTHREAD_MUTEX_INITIALIZER;
        // 懒汉模式 双检查加锁  提高效率 避免每次获取对象都要加锁
        if (_ins == nullptr)
        {
            pthread_mutex_lock(&mutx);
            if (_ins == nullptr)
            {
                _ins = new TcpServer(port);
               // _ins->InitTcpServer();
            }
            pthread_mutex_unlock(&mutx);
        }
        return _ins;
    }

    // 创建套接字
    int Socket()
    {
        _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (_listen_sock <= 0)
        {
            LogMessage(FATAL, "create sockt fail %d:%s", errno, strerror(errno));
            exit(1);
        }
        //允许在同一端口上启动服务器，即使之前的连接处于 TIME_WAIT 状态
        int opt = 1;
        setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        return _listen_sock;
    }

    // bind
    void Bind()
    {
        struct sockaddr_in src;
        memset(&src, 0, sizeof(src));
        src.sin_family = AF_INET;
        src.sin_port = htons(_port);
        src.sin_addr.s_addr = INADDR_ANY;

        int bind_ret = bind(_listen_sock, (struct sockaddr *)&src, sizeof(src));
        if (bind_ret < 0)
        {
            LogMessage(FATAL, "bind fail %d:%s", errno, strerror(errno));
            exit(2);
        }
    }

    // isten
    void Listen()
    {
        int listen_ret = listen(_listen_sock, backlog);
        if (listen_ret < 0)
        {
            LogMessage(FATAL, "listen fail %d:%s", errno, strerror(errno));
            exit(3);
        }
    }
    int Sock()
    {
        return _listen_sock;
    }
    ~TcpServer()
    {
        //关闭套接字
        if (_listen_sock >= 0)
        {
            close(_listen_sock);
        }
    }

private:
    uint16_t _port;         // 端口号
    int _listen_sock;       // 监听套接字
    static TcpServer *_ins; // 单例模式对象
};

TcpServer *TcpServer::_ins = nullptr;
