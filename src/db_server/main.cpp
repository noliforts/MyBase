#include "db_server/tcp_server.h"
#include "db_server/protocol.h"
#include "db_core/database_manager.h"
#include "db_core/storage_engine.h" // Подключаем ваш заголовок движка
#include <memory>

int main(int argc, char** argv) {
    // 1. Инициализируем хранилище СУБД
    auto& db_mgr = DatabaseManager::instance();
    db_mgr.setStorageEngine(std::make_unique<JsonFileStorageEngine>());
    db_mgr.loadAll();

    // 2. Собираем сервер из слоев и запускаем его
    int port = 9000;
    auto binary_protocol = std::make_unique<BinaryProtocol>();

    TcpServer server(port, std::move(binary_protocol));
    server.start();

    return 0;
}
