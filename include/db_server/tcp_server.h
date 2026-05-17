#pragma once
#include "protocol.h"
#include "db_server/request_handler.h"
#include <memory>

class TcpServer {
public:
    TcpServer(int port, std::unique_ptr<Protocol> protocol);
    ~TcpServer();

    void start();

private:
    int port_;
    int server_fd_;
    std::unique_ptr<Protocol> protocol_;
    SqlRequestHandler handler_;
};
