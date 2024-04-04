#include "Http_Server.hpp"
#include "Daemon.hpp"
#include <memory>
int main()
{
    Daemon();
    std::shared_ptr<HttpServer> http_server(new HttpServer(8081));
    http_server->Start();
    return 0;
}