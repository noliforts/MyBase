#include "db_core/database.h"
#include "db_core/csv_storage_engine.h"

void Database::createTable(const std::string& t_name, const TableSchema& schema) {
    if (tables.count(t_name)) throw std::runtime_error("Table already exists: " + t_name);
    tables[t_name] = Table(schema);
}

void Database::dropTable(const std::string& t_name) {
    if (!tables.erase(t_name)) throw std::runtime_error("Table not found: " + t_name);
}

Table& Database::getTable(const std::string& t_name) {
    auto it = tables.find(t_name);
    if (it == tables.end()) throw std::runtime_error("Table not found: " + t_name);
    return it->second;
}

const Table& Database::getTable(const std::string& t_name) const {
    auto it = tables.find(t_name);
    if (it == tables.end()) throw std::runtime_error("Table not found: " + t_name);
    return it->second;
}
void Database::exportTableToCsv(const std::string& t_name, const std::string& filePath) const {
    auto it = tables.find(t_name);
    if (it == tables.end()) {
        throw std::runtime_error("Execution error: Table '" + t_name + "' not found in database '" + name + "'.");
    }

    CsvStorageEngine::exportTable(it->second, filePath);
}

void Database::importTableFromCsv(const std::string& t_name, const std::string& filePath) {
    auto it = tables.find(t_name);
    if (it == tables.end()) {
        throw std::runtime_error("Execution error: Table '" + t_name + "' not found in database '" + name + "'.");
    }

    CsvStorageEngine::importTable(it->second, filePath);
}
