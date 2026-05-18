#include "db_server/protocol.h"
#include <sstream>

Request BinaryProtocol::parseRequest(const std::vector<uint8_t>& raw) {
    return {std::string(raw.begin(), raw.end())};
}

std::vector<uint8_t> BinaryProtocol::serializeResponse(const Response& resp) {
    std::string data;

    if (resp.isError) {
        data = "ERROR: " + resp.message + "\n";
        return {data.begin(), data.end()};
    }

    if (resp.isSelect) {
        for (const auto& col : resp.columns) data += col + "\t";
        data += "\n";

        for (const auto& col : resp.columns) data += std::string(col.size(), '-') + "\t";
        data += "\n";

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
    } else if (resp.isDDL) {
        data = resp.message + "\n";
    } else {
        data = "Affected rows: " + std::to_string(resp.affectedRows) + "\n";
    }

    return {data.begin(), data.end()};
}
