#pragma once
#include <variant>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstddef>

// Перечисление поддерживаемых типов данных в таблицах
enum class DataType { INT, FLOAT, BOOL, TEXT, VARCHAR };

// Структура для описания одной колонки (Имя + Тип)
struct ColumnSchema {
    std::string name;
    DataType type;
};

// Структура для описания схемы всей таблицы (Список колонок)
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

// Результат выполнения любого SQL запроса для передачи по сети
struct QueryResult {
    bool isSelect = false;
    std::vector<std::string> columns;
    std::vector<DataType> columnTypes;
    std::vector<Row> rows;
    size_t affectedRows = 0;
    std::string message;
    bool isError = false;
};
