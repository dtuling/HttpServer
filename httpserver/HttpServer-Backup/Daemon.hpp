#pragma once
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
// 守护进程
void Daemon()
{
    // 1.忽略SIGPIPE 和SIGCHLD信号
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    // 2. 让父进程退出(守护进程不能是当前进程组的组长)
    if (fork() > 0)
    {
        exit(0);
    }
    //3. 调用setsid让子进程成为守护进程
    setsid();
    //4. 守护进程不能在向终端中打印，Linux下dev/null文件是一个文件黑洞，所有文件都可以向这里显示，不影响终端正常运行
    int devnull = open("/dev/null", O_RDONLY | O_WRONLY);

    if (devnull > 0)
    {
        dup2(0, devnull);
        dup2(1, devnull);
        dup2(2, devnull);
        close(devnull);
    }
}