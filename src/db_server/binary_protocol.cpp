#include "db_server/protocol.h"
#include <sstream>

Request BinaryProtocol::parseRequest(const std::vector<uint8_t>& raw) {
    return {std::string(raw.begin(), raw.end())};
}

std::vector<uint8_t> BinaryProtocol::serializeResponse(const Response& resp) {
    // В структуре QueryResult флаг ошибки идет последним. Допустим, он называется is_error
    std::string data = resp.isError ? "ERROR\n" : "OK\n";

    // Подставляем верные поля структуры (message и count/affected_rows)
    data += resp.message + "\nRows: " + std::to_string(resp.affectedRows) + "\n";

    for (const auto& row : resp.rows) {
        for (const auto& val : row) {
            std::visit([&data](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>) data += arg + "\t";
                else if constexpr (std::is_same_v<T, std::nullptr_t>) data += "NULL\t";
                else data += std::to_string(arg) + "\t";
            }, val);
        }
        data += "\n";
    }
    return {data.begin(), data.end()};
}
