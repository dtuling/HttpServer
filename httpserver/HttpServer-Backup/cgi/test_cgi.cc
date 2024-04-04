#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include "../log.hpp"
using namespace std;
bool GetQuerySteing(std::string &query_string)
{
    std::string method = std::getenv("METHOD");
    LogMessage(DEBUG,"CGI METHOD = %s",method.c_str());
    if (method == "GET")
    {
        query_string = std::getenv("QUERY_STRING");
    }
    else if (method == "POST")
    {
        
        int length = atoi(getenv("CONTENG_LENGTH"));
        LogMessage(DEBUG,"POST CONTENG-LENGTH = %d",length);
        char ch = 0;
        while (1)
        {
            while (length)
            {
                read(0, &ch, 1);
                query_string.push_back(ch);
                length--;
            }
            if(!query_string.empty()) break;
        }
        LogMessage(DEBUG,"READ END : %s",query_string.c_str());
    }
    else
    {
        return false;
    }
    return true;
}

void CutString(std::string &target, std::string &sub_str1, std::string &sub_str2, std::string sep)
{
    size_t pos = target.find(sep);
    if (pos != std::string::npos)
    {
        sub_str1 = target.substr(0, pos);
        sub_str2 = target.substr(pos + sep.size());
    }
}
int main()
{
    // close(output[1]);
    // close(input[0]);
    // CGI程序 0读1写
    // cout 已经被重定向到管道中了,3号文件描述符还没有被占用,使用cerr进行测试
    // 获取环境变量
    //std::cerr << "CGI proceee #: " << endl;
    LogMessage(DEBUG,"CGI PROCESS EXECL SUCCESS!!!");
    std::string query_string;

    // 获取参数 GET /test_cgi?x=100&y=200 => str1==>x=100 str2=>y=200
    if (GetQuerySteing(query_string))
    {
        //cerr << query_string << endl;
        LogMessage(DEBUG,"QUERY = %s",query_string.c_str());
        std::string str1;
        std::string str2;
        CutString(query_string, str1, str2, "&");
        std::string name1;
        std::string value1;
        CutString(str1, name1, value1, "=");
        std::string name2;
        std::string value2;
        CutString(str2, name2, value2, "=");
        // 向管道中发送  已经重定向了 CGI处理完 发到管道 让httpserver进行处理响应
        cout << name1 << ":" << value1 << endl;
        cout << name2 << ":" << value2 << endl;
        // 打印信息
        cerr << name1 << ":" << value1 << endl;
        cerr << name2 << ":" << value2 << endl;
    }

    return 0;
}