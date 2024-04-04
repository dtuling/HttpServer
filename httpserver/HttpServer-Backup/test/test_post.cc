#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // 指定服务器地址和端口
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8081);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to connect to server" << std::endl;
        close(sockfd);
        return 1;
    }

    // 构造 HTTP POST 请求
    std::string post_data = "data1=value1&data2=value2";
    std::string request = "POST /test_cgi HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: application/x-www-form-urlencoded\r\n"
                          "Content-Length: " + std::to_string(post_data.length()) + "\r\n"
                          "\r\n" + post_data;

    // 发送请求
    if (send(sockfd, request.c_str(), request.length(), 0) == -1) {
        std::cerr << "Failed to send request" << std::endl;
        close(sockfd);
        return 1;
    }

    // 接收响应并输出
    char buffer[1024];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
    if (bytes_received == -1) {
        std::cerr << "Failed to receive response" << std::endl;
    } else {
        std::cout << "Response received: " << std::string(buffer, bytes_received) << std::endl;
    }

    // 关闭套接字
    close(sockfd);

    return 0;
}
