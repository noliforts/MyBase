#include "db_core/condition.h"

bool ComparisonNode::evaluate(const Row& row, const TableSchema& schema) const {
    size_t idx = schema.getColumnIndex(column);
    const Value& cell = row[idx];

    // 1. Рантайм-обработка: если хотя бы одна сторона NULL
    if (std::holds_alternative<std::nullptr_t>(cell) || std::holds_alternative<std::nullptr_t>(value)) {
        bool cellIsNull = std::holds_alternative<std::nullptr_t>(cell);
        bool valIsNull = std::holds_alternative<std::nullptr_t>(value);

        if (op == "=") return cellIsNull && valIsNull;
        if (op == "!=") return cellIsNull != valIsNull;
        return false; // Любые <, >, <=, >= с NULL возвращают false
    }

    // 2. Безопасный визитор с разделением логики на этапе компиляции
    return std::visit([this](auto&& lhs, auto&& rhs) -> bool {
        using T = std::decay_t<decltype(lhs)>;
        using U = std::decay_t<decltype(rhs)>;

        if constexpr (std::is_same_v<T, U>) {
            // Если компилятор генерирует ветку для nullptr, мы жестко подставляем значения
            // и НЕ компилируем операторы сравнения <, >, <=, >=
            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                if (op == "=") return true;
                if (op == "!=") return false;
                return false;
            } else {
                // Для обычных типов (int, float, string, bool) компилируем как обычно
                if (op == "=") return lhs == rhs;
                if (op == "!=") return lhs != rhs;
                if (op == "<") return lhs < rhs;
                if (op == ">") return lhs > rhs;
                if (op == "<=") return lhs <= rhs;
                if (op == ">=") return lhs >= rhs;
            }
        }
        return false;
    }, cell, value);
}
