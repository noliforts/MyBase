#include "db_client_lib/db_connection.h"
#include <iostream>

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 9000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) host = argv[++i];
        if (arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
    }

    TcpDbConnection conn(host, port);
    std::cout << "Connected to " << host << ":" << port << "\n";

    std::string query;
    while (true) {
        std::cout << "mydb> ";
        std::getline(std::cin, query);
        if (query == "exit" || query == "quit") break;
        if (query.empty()) continue;

        DbResult res = conn.execute(query);
        std::cout << res.output << "\n";
    }
    return 0;
}
