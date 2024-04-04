#pragma once
#include "Protocol.hpp"
class Task
{
public:
    Task() {}
    Task(int sock) : _sock(sock) {}
    //处理任务
    void ProcessOn()
    {
        handler(_sock);//CallBack中重载了()
    }

private:
    int _sock;
    CallBack handler; // 设置回调 
};