#include "db_core/storage_engine.h"
#include "db_core/database_manager.h"
#include "json.hpp"
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

static std::string dataTypeToString(DataType type) {
    switch (type) {
        case DataType::INT:     return "INT";
        case DataType::FLOAT:   return "FLOAT";
        case DataType::BOOL:    return "BOOL";
        case DataType::TEXT:    return "TEXT";
        case DataType::VARCHAR: return "VARCHAR";
    }
    return "TEXT";
}

static DataType stringToDataType(const std::string& str) {
    if (str == "INT")     return DataType::INT;
    if (str == "FLOAT")   return DataType::FLOAT;
    if (str == "BOOL")    return DataType::BOOL;
    if (str == "TEXT")    return DataType::TEXT;
    if (str == "VARCHAR") return DataType::VARCHAR;
    return DataType::TEXT;
}

static json valueToJson(const Value& value) {
    return std::visit([](auto&& arg) -> json {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return nullptr;
        } else {
            return arg;
        }
    }, value);
}

static Value jsonToValue(const json& j, DataType type) {
    if (j.is_null()) return nullptr;
    switch (type) {
        case DataType::INT:     return j.get<int>();
        case DataType::FLOAT:   return j.get<float>();
        case DataType::BOOL:    return j.get<bool>();
        case DataType::TEXT:
        case DataType::VARCHAR: return j.get<std::string>();
    }
    return nullptr;
}

void JsonFileStorageEngine::saveDatabase(const Database& db, const std::string& path) {
    for (const auto& [tableName, table] : db.getTables()) {
        std::string filePath = path + "/" + db.getName() + "_" + tableName + ".jsonl";
        std::ofstream file(filePath);
        if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filePath);

        json schemaJson = json::array();
        for (const auto& col : table.schema.columns) {
            schemaJson.push_back({ {"name", col.name}, {"type", dataTypeToString(col.type)} });
        }
        file << schemaJson.dump() << "\n";

        for (const auto& row : table.getRows()) {
            json rowJson = json::array();
            for (const auto& cell : row) {
                rowJson.push_back(valueToJson(cell));
            }
            file << rowJson.dump() << "\n";
        }
    }
}

Database JsonFileStorageEngine::loadDatabase(const std::string& path) {
    throw std::runtime_error("Use loadAll/saveAll for database manager context instead.");
}

void JsonFileStorageEngine::saveAll(const DatabaseManager& mgr) {
    std::string basePath = "./data"; 
    for (const auto& [dbName, db] : mgr.getDatabases()) {
        saveDatabase(db, basePath);
    }
}

void JsonFileStorageEngine::loadAll(DatabaseManager& mgr) {
    std::string basePath = "./data";
    
    for (const auto& dbName : mgr.getDatabaseNames()) {
        Database db(dbName);
        
        for (const auto& tableName : mgr.getTableNamesForDb(dbName)) {
            std::string filePath = basePath + "/" + dbName + "_" + tableName + ".jsonl";
            std::ifstream file(filePath);
            if (!file.is_open()) continue;

            std::string line;
            TableSchema schema;
            std::vector<Row> rows;

            if (std::getline(file, line) && !line.empty()) {
                json schemaJson = json::parse(line);
                for (const auto& colJ : schemaJson) {
                    schema.columns.push_back({
                        colJ["name"].get<std::string>(),
                        stringToDataType(colJ["type"].get<std::string>())
                    });
                }
            }

            while (std::getline(file, line)) {
                if (line.empty()) continue;
                json rowJson = json::parse(line);
                Row row;
                for (size_t i = 0; i < rowJson.size(); ++i) {
                    row.push_back(jsonToValue(rowJson[i], schema.columns[i].type));
                }
                rows.push_back(row);
            }
            
            db.addTable(tableName, schema, rows);
        }
        mgr.addDatabase(std::move(db));
    }
}