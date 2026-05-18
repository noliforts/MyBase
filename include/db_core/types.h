#pragma once
#include <variant>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstddef>

enum class DataType { INT, FLOAT, BOOL, TEXT, VARCHAR };

struct ColumnSchema {
    std::string name;
    DataType type;
};

struct TableSchema {
    std::vector<ColumnSchema> columns;

    size_t getColumnIndex(const std::string& colName) const {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].name == colName) return i;
        }
        throw std::runtime_error("Column not found: " + colName);
    }

    size_t columnCount() const {
            return columns.size();
        }
};


using Value = std::variant<int, float, bool, std::string, std::nullptr_t>;
using Row = std::vector<Value>;

struct QueryResult {
    bool isSelect = false;
    bool isDDL = false;
    std::vector<std::string> columns;
    std::vector<DataType> columnTypes;
    std::vector<Row> rows;
    size_t affectedRows = 0;
    std::string message;
    bool isError = false;
};
