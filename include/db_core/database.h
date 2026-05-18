#pragma once
#include "table.h"
#include <map>

class Database {
public:
    std::string name;
    std::map<std::string, Table> tables;

    Database() = default;
    explicit Database(std::string n) : name(std::move(n)) {}

    void createTable(const std::string& t_name, const TableSchema& schema);
    void dropTable(const std::string& t_name);
    Table& getTable(const std::string& t_name);
    const Table& getTable(const std::string& t_name) const;

    void exportTableToCsv(const std::string& t_name, const std::string& filePath) const;
    void importTableFromCsv(const std::string& t_name, const std::string& filePath);
};
