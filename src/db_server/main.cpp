#include "db_server/tcp_server.h"
#include "db_server/protocol.h"
#include "db_core/database_manager.h"
#include "db_core/storage_engine.h" // Подключаем ваш заголовок движка
#include <memory>

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 9000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) host = argv[++i];
        else if (arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
    }

    auto& db_mgr = DatabaseManager::instance();
    db_mgr.setStorageEngine(std::make_unique<JsonFileStorageEngine>());
    db_mgr.loadAll();

    TcpServer server(host, port, std::make_unique<BinaryProtocol>());
    server.start();

    return 0;
}
