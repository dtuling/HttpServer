#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

class Util
{
public:
        static int ReadLine(int sock, std::string &out)
        {
            char ch = '0';
            while (ch != '\n')
            {
                ssize_t recv_size = recv(sock, &ch, 1, 0); // 每次读取一个字符
                if (recv_size > 0)
                {
                    if (ch == '\r')
                    {
                        // 窥探下一个字符是否为\n MSG_PEEK可以窥探缓冲区中下一个字符 能做到只看不读
                        recv(sock, &ch, 1, MSG_PEEK);
                        if (ch == '\n')
                        {
                            recv(sock, &ch, 1, 0); // 读走\n
                        }
                        else
                        {
                            ch = '\n'; // 不是\n设置为\n
                        }
                    }
                    out.push_back(ch);
                }
                else if (recv_size == 0)// 对方关闭连接
                { 
                    return 0;
                }
                else// 读取失败
                { 
                    return -1;
                }
            }
            return out.size(); // 返回读取到的个数
        }
    // eg: target = hello:world
    //     sep = :
    //     sub1_str = hello
    //     sub2_str = world
    static bool cutString(const std::string &target, std::string &sub1_str, std::string &sub2_str, const std::string sep)
    {
        size_t pos = target.find(sep);
        if (pos != std::string::npos)
        {
            sub1_str = target.substr(0, pos);
            sub2_str = target.substr(pos + sep.size()); // 默认到结尾
            return true;
        }
        return false;
    }
};
