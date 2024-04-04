// 日志功能
#pragma once
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdarg.h>

#define DEBUG 0
#define NORMAL 1
#define WARING 2
#define ERROR 3
#define FATAL 4

#define FILE_NAME "./log.txt"
// 日志等级
const char *gLevelMap[] = {
    "DEBUGE",
    "NORMAL",
    "WARING",
    "ERROR",
    "FATAL"};
// 完整的日志功能，至少: 日志等级 时间 支持用户自定义(日志内容, 文件行，文件名)
void LogMessage(int level, const char *format, ...)
{
    // 日志标准部分
    char stdbuffer[1024];
    time_t get_time = time(nullptr);

    // 标准部分的等级和时间
    struct tm *info = localtime(&get_time);
    snprintf(stdbuffer, sizeof(stdbuffer), "[%s]|[%d:%d:%d:%d:%0d]",
             gLevelMap[level], info->tm_year + 1900, info->tm_mon + 1, info->tm_mday,
             info->tm_hour,info->tm_min);

    // 日志自定义部分
    char logbuffer[1024];
    // 获取可变参数列表
    va_list valist;
    // 为 num 个参数初始化 valist
    va_start(valist, format);
    vsnprintf(logbuffer, sizeof(logbuffer), format, valist);
    // 清理为 valist 保留的内存
    va_end(valist);

    // 打印完整的日志信息(可重定向到日志文件中)
    //std::cout << stdbuffer << logbuffer << std::endl;

    // 把日志信息记录到日志文件中
      FILE* fp = fopen(FILE_NAME,"a");
      fprintf(fp,"%s %s\n",stdbuffer,logbuffer);
      fclose(fp);
}