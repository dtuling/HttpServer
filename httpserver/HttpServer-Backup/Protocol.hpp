#pragma once

#include <iostream>
#include <string>
#include <pthread.h>
#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <unordered_map>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <algorithm>
#include "Util.hpp"

using std::cout;
using std::endl;

#define SEP ": "          // 报文中的分隔符
#define WWWROOT "wwwroot" // 指定默认根路径
#define HOME_PAGE "index.html"
#define PAGE_404 "404.html"
#define OK 200
#define NOT_FOUND 404
#define BADN_REQUEST 400
#define HTTP_VERSION "HTTP/1.0"
// http协议 content-type对照表
static std::string TypeToDesc(std::string str)
{
    std::unordered_map<std::string, std::string> compare_table = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".jpg", "application/x-jpg"},
        {".xml", "application/xml"},
    };

    auto it = compare_table.find(str);
    if (it != compare_table.end())
    {
        return it->second;
    }
    return "text/html";
}
// 转态码转状态码描述
static std::string CodeToDesc(int code)
{
    std::string desc;
    switch (code)
    {
    case 200:
        desc = "OK";
        break;
    case 400:
        desc = "NOT FOUND";
        break;
    default:
        desc = "UNKNOWN";
        break;
    }
    return desc;
}
//   http请求
struct HttpRequest
{
    std::string request_line;                // 请求行
    std::vector<std::string> request_header; // 请求报文
    std::string blank = "\r\n";              // 请求空行
    std::string request_body;                // 请求正文

    // 解析请求行
    std::string method;  // 请求方法
    std::string uri;     // 请求uri
    std::string version; // http版本
    std::string suffix;  // 请求资源后缀

    // 解析请求报文
    std::unordered_map<std::string, std::string> header_kv;

    int content_length; // 请求正文字节数

    // GET /a/b.index.html?x=100&y=100  http1.0
    std::string path;        // 请求路径(/a/b.index.html)
    std::string quer_string; // 请求参数(x=100&y=100)

    bool cgi = false; // 是否使用cgi机制
    int size;         // 目标资源文件大小
};

// http响应
struct HttpResponse
{
    std::string status_line;                  // 状态行
    std::vector<std::string> response_header; // 响应报文
    std::string blank = "\r\n";               // 响应空行
    std::string respinse_body;                // 响应正文

    int status_code; // 响应状态码

    int fd; // 目标资源文件描述符
};

// 读取请求  分析处理请求 构建响应
class EndPoint
{
private:
    // 读取请求行 http不同平台下的分隔符可能不一样，可能是\r 或 \n 在或者\r\n;？？？
    // 在Util类中提供一个按行读取的方法进行解耦
    bool RecvHttpRequestLine()
    {
        std::string &line = _http_request.request_line;
        int read_size = Util::ReadLine(_sock, line);
        if (read_size > 0)
        {
            line.resize(line.size() - 1);
        }
        else
        {
            _stop = true;
        }
        return _stop;
    }

    // 读取请求报头和空行
    bool RecvHttpRequestHeader()
    {
        std::string line;
        while (true)
        {
            line.clear(); // 每次读保文时清空一下
            if (Util::ReadLine(_sock, line) <= 0)
            {
                _stop = true; // 读中断了
                break;
            }
            if (line == "\n") // 读到空行
            {
                _http_request.blank = line;
                break;
            }
            // 将读取到的每一行插入到request_header中(去掉\n)
            line.resize(line.size() - 1);
            _http_request.request_header.push_back(line);
        }
        return _stop;
    }

    // 解析请求行
    void ParseHttpRequestLine()
    {
        std::string &line = _http_request.request_line;
        // 通过stringstream拆分请求行
        // eg:  GET / HTTP/1.1 拆分为三部分 请求方法  url 和http版本
        std::stringstream ss(line);
        ss >> _http_request.method >> _http_request.uri >> _http_request.version;
        // http协议不区分大小写 这里请求方法全部统一为大写
        auto &method = _http_request.method;
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        LogMessage(DEBUG, "METHOD=%s,URI=%s,VERSION=%s", _http_request.method.c_str(), _http_request.uri.c_str(), _http_request.version.c_str());
    }
    // 解析请求报文
    void ParseHttpRequestHeader()
    {
        std::string key;
        std::string value;
        // 将读到的报文 以K/V结构存储到unordered_map中
        for (auto &iter : _http_request.request_header)
        {
            // 报文中的数据以: 做为分隔符 以: 切割子串
            if (Util::cutString(iter, key, value, SEP))
            {
                _http_request.header_kv.insert({key, value});
            }
        }
    }

    // 判断是否需要读取请求正文
    bool IsNeedRecvHttpRequestBody()
    {
        // POST方法才需要读取正文
        auto &method = _http_request.method;

        if (method == "POST") // 已经处理过 这里不区分大小写
        {
            // 根据报文中的信息获取正文的字节数
            auto &header_kv = _http_request.header_kv;
            auto iter = header_kv.find("Content-Length");
            if (iter != header_kv.end())
            {
                _http_request.content_length = atoi(iter->second.c_str());
            }
            return true;
        }
        return false;
    }
    // 请求正文
    void RecvHttpRequestBody()
    {
        // 判断正文是否有需求读取
        if (IsNeedRecvHttpRequestBody())
        {
            int length = _http_request.content_length; // 别用引用，否则正文长度为0了。后面向CGI进程传数据时会出问题
            std::string &body = _http_request.request_body;
            char ch = 0;
            while (length)
            {
                ssize_t recv_size = recv(_sock, &ch, 1, 0);
                if (recv_size > 0)
                {
                    body += ch;
                    length--;
                }
                else
                {
                    break;
                }
            }
            LogMessage(DEBUG, "REQUEST BODY = %s", _http_request.request_body.c_str());
        }
    }
    void BuilOkResponse()
    {
        // 构建响应报头
        std::string line = "Content-Type: ";
        line += TypeToDesc(_http_request.suffix);
        line += "\r\n";
        _http_response.response_header.push_back(line);

        line = "Content-Length: ";
        if (_http_request.cgi)
        {
            line += std::to_string(_http_request.request_body.size());
        }
        else
        {
            line += std::to_string(_http_request.size);
        }
        line += "\r\n";
        _http_response.response_header.push_back(line);
    }
    // 构建错误响应报头和正文
    void BuilErrorResponse()
    {
        // 返回404页面即可
        std::string path = WWWROOT;
        path += "/";
        path += PAGE_404;
        _http_response.fd = open(path.c_str(), O_RDONLY);
        if (_http_response.fd > 0)
        {
            struct stat statbuff;
            stat(path.c_str(), &statbuff);
            _http_request.size = statbuff.st_size;

            std::string line = "Content-Type: text/html";
            line += "\r\n";
            _http_response.response_header.push_back(line);

            line = "Content-Length: ";
            line += std::to_string(_http_request.size);
            _http_response.response_header.push_back(line);
        }
    }
    void BuildHttpResponseHelper()
    {

        int &code = _http_response.status_code;
        LogMessage(DEBUG, "RESPONSE CODE = %d", code);
        // 构建响应行
        std::string &line = _http_response.status_line;
        line += HTTP_VERSION;
        line += " ";
        line += std::to_string(code);
        line += " ";
        line += CodeToDesc(code);
        line += "\r\n"; // 空行

        // 构建响应正文,可能包括响应报头
        // 根据状态码构建响应报头和响应正文
        switch (code)
        {
        case OK:
            BuilOkResponse();
            break;
        case NOT_FOUND:
            BuilErrorResponse();
            break;
        default:
            break;
        }
    }

    // 非CGI返回静态网页即可
    int ProcessNoCGI()
    {
        LogMessage(DEBUG, "NOT USE CGI------------------------");
        _http_response.fd = open(_http_request.path.c_str(), O_RDONLY);
        if (_http_response.fd < 0)
        {
            return NOT_FOUND;
        }
        return OK;
    }

    // GET带参或者POST方法请求时需要使用CGI机制
    int ProcessCGI()
    {
        LogMessage(DEBUG, "USE CGI -----------------------");
        std::string &bin = _http_request.path; // CGI程序路径
        LogMessage(DEBUG, "CGI PROCESS BIN = %s", bin.c_str());
        std::string &method = _http_request.method; // 请求方法
        int code = OK;                              // 响应状态码
        // 创建两个管道进行httpserver和CGI进程进行通信(父子进程进行通信)
        // 管道是单向通信的  这里需要两个管道进行读写
        int input[2];
        int output[2];

        if (pipe(input) < 0)
        {
            LogMessage(FATAL, "CREATE INPIPE FAIL");
            code = 404;
            return code;
        }
        if (pipe(output) < 0)
        {
            LogMessage(FATAL, "CREATE OUTPIPE FAIL");
            code = 404;
            return code;
        }

        // 创建子进程进程程序替换CGI程序
        pid_t pid = fork();
        if (pid == 0) // 子进程
        {
            close(output[1]);
            close(input[0]);

            // execl进程程序替换 不会替换环境变量 继续使用子进程的环境变量

            std::string method_env = "METHOD="; // 导入METHOD环境变量
            method_env += method;
            putenv((char *)method_env.c_str());

            if (method == "GET")
            {
                // GET 方法需要知道uri中的参数 通过环境变量导入 CGI程序使用getenv获取参数
                std::string &query_string = _http_request.quer_string; // 请求参数
                std::string query_string_env = "QUERY_STRING=";
                query_string_env += query_string;
                putenv((char *)query_string_env.c_str());
                LogMessage(NORMAL, "METHOD = GET, ADD QUERY_STRING ENV");
            }
            else if (method == "POST")
            {
                // POST 方法需要知道请求正文中的字节数，也通过环境变量导入
                std::string content_length_env = "CONTENG_LENGTH=";
                content_length_env += std::to_string(_http_request.content_length);
                putenv((char *)content_length_env.c_str());
                LogMessage(NORMAL, "METHOD = POST, ADD CONTENG_LENGTH ENV");
            }
            else
            {
                //...
            }
            // 替换前重定向
            dup2(output[0], 0);
            dup2(input[1], 1);

            // 进程程序替换,代码和数据都会被替换了,子进程拿到的管道文件描述符已经被替换走了,使用重定向让目标替换进行0读1写(每个进程都会打开0,1,2)
            execl(bin.c_str(), bin.c_str(), nullptr);
            exit(0);
        }
        else if (pid < 0)
        {
            LogMessage(FATAL, "create subprocess fail");
            code = 404;
            return code;
        }
        else // 父进程
        {

            close(output[0]);
            close(input[1]);

            if (method == "POST") // POST方法，需要父进程将正文中的参数通过管道发送给CGI程序
            {
                std::string &request_body = _http_request.request_body;
                write(output[1], request_body.c_str(), request_body.size()); // 数据太大一次性可能写不完 待解决
            }

            // 父进程从管道中读取数据(CGI向管道中写数据了) 读完存到响应正文中 返回给客户端
            char ch = 0;
            while (read(input[0], &ch, 1) > 0)
            {
                _http_response.respinse_body.push_back(ch);
            }

            int status;
            pid_t wait_pid = waitpid(pid, &status, 0);
            if (wait_pid == pid && WIFEXITED(status)) // 等待成功
            {
                // #define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
                if (WEXITSTATUS(status) == 0)
                {
                    code = OK;
                }
                else // 子进程异常退出
                {
                    code = 400;
                }
            }
            else // 等待失败
            {
                code = 500;
            }

            close(output[1]);
            close(input[0]);
        }
        return code;
    }
    // 请求合法性
    bool RequesIstAllow()
    {
        int &code = _http_response.status_code;
        if (_http_request.method == "GET") // 通过url传参
        {

            // 路径和参数使用？分隔  默认是没有参数的。
            size_t pos = _http_request.uri.find('?');
            if (pos != std::string::npos)
            {
                // 切割url 将路径和参数提取
                Util::cutString(_http_request.uri, _http_request.path, _http_request.quer_string, "?");
                // GET 方法带参了 也是也CGI机制处理
                _http_request.cgi = true;
            }
            else
            {
                _http_request.path = _http_request.uri;
            }
        }
        else if (_http_request.method == "POST")
        {
            // 使用CGI机制处理 CGI程序的路径就在uri里面
            _http_request.cgi = true;
            _http_request.path = _http_request.uri;
        }
        else // 非法请求
        {
            LogMessage(DEBUG, "NOT ALLOW REQUEST");
            code = BADN_REQUEST;
            return false;
        }
        return true;
    }
    // 判断请求资源是否存在 使用SYSTEAM CALL stat(检测指定路径下文件属性)
    bool ResourceExist()
    {
        struct stat statbuf; // 存储文件属性struct
        auto &code = _http_response.status_code;
        if (stat(_http_request.path.c_str(), &statbuf) == 0)
        {
            // 资源存在->判断资源文件的属性(目录/.exe都有可能  )
            if (S_ISDIR(statbuf.st_mode)) // 目录 返回该目录下的index.html即可
            {
                _http_request.path += '/'; // path = wwwroot/a/b 要加分隔符
                _http_request.path += HOME_PAGE;
                stat(_http_request.path.c_str(), &statbuf); // 更新一下资源文件 获取大小
            }
            else if ((statbuf.st_mode & S_IXUSR) || (statbuf.st_mode & S_IXGRP) || (statbuf.st_mode & S_IXOTH)) //.exe 使用CGI机制进行处理
            {
                _http_request.cgi = true;
            }
            else if (S_ISREG(statbuf.st_mode)) // 普通文件
            {
                //....
            }
            else
            {
                LogMessage(ERROR, "UNKONW RESOURCE FILE STAT");
            }
            _http_request.size = statbuf.st_size; // 资源文件的大小
        }
        else
        {
            // 资源不存在
            LogMessage(ERROR, "REQUEST RESOURCE NOT EXIST");
            code = NOT_FOUND;
            return false;
        }
        return true;
    }
    //  获取请求数据类型 根据后缀suffix 进行确定
    void ResourceSuffix()
    {
        size_t pos = _http_request.path.rfind(".");
        if (pos == std::string::npos)
        {
            _http_request.suffix = ".html";
        }
        else
        {
            _http_request.suffix = _http_request.path.substr(pos);
        }
    }

public:
    EndPoint(int sock) : _sock(sock) {}
    // 读取请求
    void RecvHttpRequest()
    {
        // 读取请求行 // 读取请求报头
        if ((!RecvHttpRequestLine()) && (!RecvHttpRequestHeader()))
        {
            ParseHttpRequestLine();   // 解析请求行
            ParseHttpRequestHeader(); // 解析请求报头
            RecvHttpRequestBody();    // 读取正文
        }
    }

    // 构建响应   // 已经读完请求 构建响应
    void BuildHttpResponse()
    {
        int &code = _http_response.status_code;
        // 判断请求的合法性
        if (RequesIstAllow())
        {
            // 指定web根目录
            std::string path = _http_request.path;
            _http_request.path = WWWROOT;
            _http_request.path += path;
            // 访问默认界面 比如http://8.130.46.184:8081 请求路径为wwwroot/
            if (_http_request.path[_http_request.path.size() - 1] == '/')
            {
                _http_request.path += HOME_PAGE;
                code = OK;
            }
            if (ResourceExist()) // 资源存在
            {
                ResourceSuffix(); // 获取资源类型
                // 确定响应机制 CGI or NOCGI(返回静态网页即可)
                if (_http_request.cgi)
                {
                    code = ProcessCGI();
                }
                else
                {
                    code = ProcessNoCGI();
                }
            }
        }
        // 检查完毕构建响应
        BuildHttpResponseHelper();
    }
    // 发送响应报文
    void SendHttpResponse()
    {
        //// 发送状态行
        if (send(_sock, _http_response.status_line.c_str(), _http_response.status_line.size(), 0) < 0)
        {
            _stop = true;
            return;
        }

        for (auto &iter : _http_response.response_header) // 发送响应报文
        {
            if (send(_sock, iter.c_str(), iter.size(), 0) < 0)
            {
                _stop = true;
                return;
            }
        }
        // 发送空行
        if (send(_sock, _http_response.blank.c_str(), _http_response.blank.size(), 0) < 0)
        {
            return;
        }

        // 发送正文(即请求报文下路径资源文件  这里使用sendfile发送，提高发送效率直接从内核层进行发送)
        sendfile(_sock, _http_response.fd, nullptr, _http_request.size);
        close(_http_response.fd);
    }

    bool IsStop()
    {
        return _stop;
    }

private:
    int _sock;                   // accept接收到套接字
    HttpRequest _http_request;   // http请求
    HttpResponse _http_response; // http响应
    bool _stop = false;          // 读取是否中断
};

class CallBack
{
public:
    void operator()(int sock)
    {
        HandlerRequest(sock);
    }
    // 读取请求
    void HandlerRequest(int sock)
    {

        // int sock = *(int* )args;
        EndPoint *ep = new EndPoint(sock);
        ep->RecvHttpRequest(); // 读取http请求
        // 处理读取错误 建立请求后 什么也没请求 不用继续后续操作了
        if (!ep->IsStop())
        {
            // ep->ParseHttpRequest();  // 解析http请求
            ep->BuildHttpResponse(); // 构建响应
            // 处理写入错误...... 系统会给进程发送sigpipe信号
            ep->SendHttpResponse(); // 发送响应
        }
        else
        {
            LogMessage(WARING, "RECV ERROR ,STOP BUILD AND SEND");
        }
        delete ep;
        close(sock);
    }
};
