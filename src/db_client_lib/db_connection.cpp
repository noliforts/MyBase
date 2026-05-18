#include "db_client_lib/db_connection.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

DbResult TcpDbConnection::execute(const std::string& query) {
    // Новый сокет на каждый запрос: сервер держит сессию только на время
    // одного соединения. Клиент компенсирует это, предваряя запрос строкой
    // "USE db;\n", чтобы оба выражения шли в одном TCP-соединении.
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_);
    inet_pton(AF_INET, host_.c_str(), &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        return {false, "Connection failed"};
    }

    send(sock, query.c_str(), query.length(), 0);
    shutdown(sock, SHUT_WR);

    std::string result;
    char buf[4096];
    ssize_t n;
    while ((n = read(sock, buf, sizeof(buf))) > 0)
        result.append(buf, n);

    ::close(sock);
    return {true, result};
}
