// 单例模式的线程池
#pragma once
#include <iostream>
#include <queue>
#include <pthread.h>
#include "log.hpp"
static const int NUM = 5;

class ThreadPool
{
public:
    //获取单例
    static ThreadPool *GetInstance()
    {
        static pthread_mutex_t inst_mutex = PTHREAD_MUTEX_INITIALIZER;
        if (_inst == nullptr)
        {
            pthread_mutex_lock(&inst_mutex);
            if (_inst == nullptr)
            {
                _inst = new ThreadPool;
                _inst->InitPthreadPool();
            }
            pthread_mutex_unlock(&inst_mutex);
        }
        return _inst;
    }
    ~ThreadPool()
    {
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);
    }
    //初始化线程池
    bool InitPthreadPool()
    {
        for (int i = 0; i < _num; ++i)
        {
            pthread_t tid;
            if (pthread_create(&tid, nullptr, ThreadRoutine, this) != 0)
            {
                LogMessage(FATAL, "CREATE THREAD POOL FAIL!!!!");
                return false;
            }
        }
        LogMessage(DEBUG, "CREATE THREAD POOL SUCCESS");
        return true;
    }
    //推送任务
    void PushTask(const Task &task)
    {
        Lock();
        _task_queue.push(task);
        Unlock();
        ThreadWakeUp();
    }
    //取任务
    void PopTask(Task& task)
    {
        task = _task_queue.front();
        _task_queue.pop();
    }
    void Lock()
    {
        pthread_mutex_lock(&_mutex);
    }
    void Unlock()
    {
        pthread_mutex_unlock(&_mutex);
    }
    void ThreadWait()
    {
        pthread_cond_wait(&_cond, &_mutex);
    }
    void ThreadWakeUp()
    {
        pthread_cond_signal(&_cond);
    }
    bool TaskQueueIsEmpty()
    {
        return _task_queue.empty();
    }
    //线程回调函数
    static void *ThreadRoutine(void *args)
    {
        ThreadPool *tp = (ThreadPool *)args;
        while (true)
        {
            Task task;
            tp->Lock();
            while (tp->TaskQueueIsEmpty())
            {
                tp->ThreadWait();
            }
            tp->PopTask(task);
            tp->Unlock();
            // 处理任务
            task.ProcessOn();
        }
    }

private:
    std::queue<Task> _task_queue; // 任务队列
    pthread_mutex_t _mutex;    // 互斥锁
    pthread_cond_t _cond;      // 条件变量
    int _num;                  // 线程数量

    static ThreadPool *_inst;
    ThreadPool(int num = NUM) : _num(num)
    {
        pthread_mutex_init(&_mutex, nullptr);
        pthread_cond_init(&_cond, nullptr);
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
};

ThreadPool* ThreadPool::_inst = nullptr;