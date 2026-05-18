#pragma once
#include "protocol.h"
#include <memory>

class TcpServer {
public:
    TcpServer(std::string host, int port, std::unique_ptr<Protocol> protocol);
    ~TcpServer();

    void start();

private:
    std::string host_;
    int port_;
    int server_fd_;
    std::unique_ptr<Protocol> protocol_;
};
