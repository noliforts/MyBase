#include "db_core/csv_storage_engine.h"

// 1. КОНВЕРТАЦИЯ: Value -> String (для экспорта)
std::string CsvStorageEngine::valueToString(const Value& val) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return "NULL";
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::string>) {
            // Оборачиваем строки в кавычки для безопасности CSV
            return "\"" + arg + "\"";
        } else { // int и float
            return std::to_string(arg);
        }
    }, val);
}

// 2. КОНВЕРТАЦИЯ: String -> Value (для импорта на основе ожидаемого DataType)
Value CsvStorageEngine::stringToValue(const std::string& str, DataType type) {
    if (str == "NULL" || str.empty()) {
        return nullptr;
    }

    switch (type) {
        case DataType::INT: // Предполагаю имя перечисления из твоих "types.h"
            return std::stoi(str);
        case DataType::FLOAT:
            return std::stof(str);
        case DataType::BOOL:
            return (str == "true" || str == "1");
        case DataType::TEXT:
        case DataType::VARCHAR: {
            // Если строка в файле обернута в кавычки, снимаем их
            std::string cleanStr = str;
            if (cleanStr.size() >= 2 && cleanStr.front() == '"' && cleanStr.back() == '"') {
                cleanStr = cleanStr.substr(1, cleanStr.size() - 2);
            }
            return cleanStr;
        }
        default:
            throw std::runtime_error("Unsupported data type for CSV conversion");
    }
}

// 3. ЭКСПОРТ ТАБЛИЦЫ В ФАЙЛ
void CsvStorageEngine::exportTable(const Table& table, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for export: " + filePath);
    }

    // Записываем заголовок (имена колонок)
    const auto& columns = table.schema.columns;
    for (size_t i = 0; i < columns.size(); ++i) {
        file << columns[i].name;
        if (i + 1 < columns.size()) file << ",";
    }
    file << "\n";

    // Записываем строки данных
    for (const auto& row : table.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << valueToString(row[i]);
            if (i + 1 < row.size()) file << ",";
        }
        file << "\n";
    }
}

// 4. ИМПОРТ ТАБЛИЦЫ ИЗ ФАЙЛА
void CsvStorageEngine::importTable(Table& table, const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for import: " + filePath);
    }

    std::string line;
    // Пропускаем первую строку (заголовки), так как схема у таблицы уже есть в памяти
    if (!std::getline(file, line)) {
        return;
    }

    const auto& columns = table.schema.columns;

    // Читаем строки с данными
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::vector<std::string> tokens = parseCsvLine(line);

        if (tokens.size() != columns.size()) {
            throw std::runtime_error("CSV Import Error: Column count mismatch in file line.");
        }

        Row newRow;
        newRow.reserve(columns.size());

        for (size_t i = 0; i < tokens.size(); ++i) {
            // Приводим строку к типу i-й колонки нашей таблицы
            newRow.push_back(stringToValue(tokens[i], columns[i].type));
        }

        // Вставляем готовую строчку прямо в твою таблицу!
        table.insert(newRow);
    }
}

// 5. НАДЕЖНЫЙ ПАРСЕР СТРОКИ CSV
// Обычный stringstream по запятой сломается, если внутри текстового поля будет запятая
// Например: 1, "Привет, мир!", 4.5. Этот метод учитывает кавычки.
std::vector<std::string> CsvStorageEngine::parseCsvLine(const std::string& line) {
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            current += c; // сохраняем кавычки, stringToValue их сам снимет
        } else if (c == ',' && !inQuotes) {
            result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    result.push_back(current);
    return result;
}
