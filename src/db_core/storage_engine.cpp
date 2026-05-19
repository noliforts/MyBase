#include "db_core/storage_engine.h"
#include "db_core/database_manager.h"
#include "json.hpp"
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <set>

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

void JsonFileStorageEngine::saveDatabase(const Database& db, const std::string& basePath) {
    std::string dbPath = basePath + "/" + db.name;
    std::filesystem::create_directories(dbPath);

    std::set<std::string> validFiles;
    for (const auto& [tableName, _] : db.tables)
        validFiles.insert(tableName + ".jsonl");

    // Удаляем файлы таблиц, которых больше нет в памяти (DROP TABLE),
    // иначе при следующей загрузке они появятся снова.
    for (const auto& entry : std::filesystem::directory_iterator(dbPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".jsonl")
            if (!validFiles.count(entry.path().filename().string()))
                std::filesystem::remove(entry.path());
    }

    for (const auto& [tableName, table] : db.tables) {
        std::string filePath = dbPath + "/" + tableName + ".jsonl";
        std::ofstream file(filePath);
        if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filePath);

        json schemaJson = json::array();
        for (const auto& col : table.schema.columns)
            schemaJson.push_back({ {"name", col.name}, {"type", dataTypeToString(col.type)} });
        file << schemaJson.dump() << "\n";

        for (const auto& row : table.rows) {
            json rowJson = json::array();
            for (const auto& cell : row)
                rowJson.push_back(valueToJson(cell));
            file << rowJson.dump() << "\n";
        }
    }
}

Database JsonFileStorageEngine::loadDatabase(const std::string& path) {
    throw std::runtime_error("Use loadAll/saveAll for database manager context instead.");
}

void JsonFileStorageEngine::saveAll(const DatabaseManager& mgr) {
    std::string basePath = "./data";
    std::filesystem::create_directories(basePath);

    std::set<std::string> validDbs;
    for (const auto& [dbName, _] : mgr.getDatabases())
        validDbs.insert(dbName);

    for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
        if (entry.is_directory() && !validDbs.count(entry.path().filename().string()))
            std::filesystem::remove_all(entry.path());
    }

    for (const auto& [dbName, db] : mgr.getDatabases())
        saveDatabase(db, basePath);
}

void JsonFileStorageEngine::loadAll(DatabaseManager& mgr) {
    std::string basePath = "./data";

    if (!std::filesystem::exists(basePath)) {
        std::filesystem::create_directories(basePath);
        return;
    }

    for (const auto& dbEntry : std::filesystem::directory_iterator(basePath)) {
        if (!dbEntry.is_directory()) continue;

        std::string dbName = dbEntry.path().filename().string();
        auto& db = mgr.getDatabases()[dbName];
        if (db.name.empty()) db.name = dbName;

        for (const auto& tableEntry : std::filesystem::directory_iterator(dbEntry.path())) {
            if (!tableEntry.is_regular_file()) continue;
            if (tableEntry.path().extension() != ".jsonl") continue;

            std::string tableName = tableEntry.path().stem().string();
            try {
                std::ifstream file(tableEntry.path());
                if (!file.is_open()) continue;

                std::string line;
                TableSchema schema;
                std::vector<Row> rows;

                if (std::getline(file, line) && !line.empty()) {
                    json schemaJson = json::parse(line);
                    for (const auto& colJ : schemaJson)
                        schema.columns.push_back({
                            colJ["name"].get<std::string>(),
                            stringToDataType(colJ["type"].get<std::string>())
                        });
                }

                while (std::getline(file, line)) {
                    if (line.empty()) continue;
                    json rowJson = json::parse(line);
                    if (rowJson.size() != schema.columns.size())
                        throw std::runtime_error("row field count does not match schema");
                    Row row;
                    for (size_t i = 0; i < rowJson.size(); ++i)
                        row.push_back(jsonToValue(rowJson[i], schema.columns[i].type));
                    rows.push_back(row);
                }

                Table restoredTable;
                restoredTable.schema = schema;
                restoredTable.rows = rows;
                db.tables[tableName] = restoredTable;

            } catch (const std::exception& e) {
                std::cerr << "[WARN] Skipping corrupt file " << tableEntry.path().filename()
                          << ": " << e.what() << "\n";
            }
        }
    }
}
