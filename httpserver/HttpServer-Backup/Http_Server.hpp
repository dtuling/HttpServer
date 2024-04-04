#pragma once
#include <pthread.h>
#include <signal.h>
#include "Tcp_server.hpp"
#include "log.hpp"
#include "Protocol.hpp"
#include "Task.hpp"
#include "ThreadPool.hpp"

using std::cout;
using std::endl;

class HttpServer
{
public:
    HttpServer(uint16_t port)
        : _port(port)
    {
        TcpServer *_tcp_server = TcpServer::GetInstance(_port);
        _sock = _tcp_server->Socket();
        _tcp_server->Bind();
        _tcp_server->Listen();
        signal(SIGPIPE, SIG_IGN); // 信号SIGPIPE需要进行忽略，如果不忽略，在写入时候，可能直接崩溃server
        LogMessage(NORMAL, "HttpServer init successful socket = %d",_sock);
    }

    void Start()
    {
       
        while (1)
        {
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);
            int sock = accept(_sock, (struct sockaddr *)&peer, &len);
            if (sock < 0)
            {
                continue;
            }
            LogMessage(DEBUG,"GET A NEW LINK SOCK = %d",sock);
            //接入线程池 向任务队列推送任务
            Task task(sock);//套接字信息是accept之后的
            ThreadPool::GetInstance()->PushTask(task);
        }
    }
    ~HttpServer()
    {
    }

private:
    uint16_t _port; // 端口
    int _sock;
};