
#pragma once
#include "table.h"
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

class CsvStorageEngine {
public:
    static void exportTable(const Table& table, const std::string& filePath);
    static void importTable(Table& table, const std::string& filePath);

private:
    // Помощник для конвертации Value в строку CSV
    static std::string valueToString(const Value& val);

    // Помощник для конвертации строки CSV в конкретный тип Value по схеме
    static Value stringToValue(const std::string& str, DataType type);

    // Помощник для парсинга одной строки CSV (с учетом возможных запятых внутри кавычек)
    static std::vector<std::string> parseCsvLine(const std::string& line);
};
