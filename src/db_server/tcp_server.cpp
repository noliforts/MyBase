#include "db_server/tcp_server.h"
#include "db_core/database_manager.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <vector>

TcpServer::TcpServer(std::string host, int port, std::unique_ptr<Protocol> protocol)
    : host_(std::move(host)), port_(port), server_fd_(-1), protocol_(std::move(protocol)) {}

TcpServer::~TcpServer() {
    if (server_fd_ != -1) {
        close(server_fd_);
    }
}

void TcpServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    if (inet_pton(AF_INET, host_.c_str(), &address.sin_addr) <= 0)
        throw std::runtime_error("Invalid host address: " + host_);
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Bind failed on " + host_ + ":" + std::to_string(port_));
    }

    if (listen(server_fd_, 3) < 0) {
        throw std::runtime_error("Listen failed");
    }

    std::cout << "Server listening on " << host_ << ":" << port_ << std::endl;

    while (true) {
        int client_socket = accept(server_fd_, nullptr, nullptr);
        if (client_socket < 0) continue;

        std::string received;
        char buf[4096];
        ssize_t n;
        while ((n = read(client_socket, buf, sizeof(buf))) > 0)
            received.append(buf, n);

        if (!received.empty()) {
            std::vector<uint8_t> raw_data(received.begin(), received.end());
            Request req = protocol_->parseRequest(raw_data);
            Response res = handler_.handle(req);
            std::vector<uint8_t> out = protocol_->serializeResponse(res);
            send(client_socket, out.data(), out.size(), 0);
        }

        close(client_socket);
        DatabaseManager::instance().saveAll();
    }
}
