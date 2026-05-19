#pragma once
#include "table.h"
#include <string>

class CsvStorageEngine {
public:
    static void exportTable(const Table& table, const std::string& filePath);
    static void importTable(Table& table, const std::string& filePath);

private:
    static std::string valueToString(const Value& val);
    static Value stringToValue(const std::string& str, DataType type);
    static std::vector<std::string> parseCsvLine(const std::string& line);
};
