#include "db_server/tcp_server.h"
#include "db_core/database_manager.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <vector>

TcpServer::TcpServer(int port, std::unique_ptr<Protocol> protocol)
    : port_(port), server_fd_(-1), protocol_(std::move(protocol)) {}

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
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Bind failed");
    }

    if (listen(server_fd_, 3) < 0) {
        throw std::runtime_error("Listen failed");
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (true) {
        int client_socket = accept(server_fd_, nullptr, nullptr);
        if (client_socket < 0) continue;

        char buffer[1024] = {0};
        ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);

        if (bytes_read > 0) {
            // 1. Сетевой буфер -> Слой Протокола (Парсинг)
            std::vector<uint8_t> raw_data(buffer, buffer + bytes_read);
            Request req = protocol_->parseRequest(raw_data);

            // 2. Слой Протокола -> Слой Бизнес-логики (Выполнение SQL)
            Response res = handler_.handle(req);

            // 3. Слой Бизнес-логики -> Слой Протокола (Сериализация)
            std::vector<uint8_t> out = protocol_->serializeResponse(res);

            // 4. Отправка клиенту
            send(client_socket, out.data(), out.size(), 0);
        }

        close(client_socket);

        // Скидываем состояние на диск после выполнения команды
        DatabaseManager::instance().saveAll();
    }
}
