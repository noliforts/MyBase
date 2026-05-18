#include "db_core/condition.h"

bool ComparisonNode::evaluate(const Row& row, const TableSchema& schema) const {
    size_t idx = schema.getColumnIndex(column);
    const Value& cell = row[idx];

    // NULL проверяется до visit: применять < или > к nullptr_t нельзя даже
    // через if constexpr, потому что компилятор всё равно инстанциирует тело
    // шаблона для каждой пары типов из variant.
    if (std::holds_alternative<std::nullptr_t>(cell) || std::holds_alternative<std::nullptr_t>(value)) {
        bool cellIsNull = std::holds_alternative<std::nullptr_t>(cell);
        bool valIsNull  = std::holds_alternative<std::nullptr_t>(value);
        if (op == "=")  return cellIsNull && valIsNull;
        if (op == "!=") return cellIsNull != valIsNull;
        return false;
    }

    return std::visit([this](auto&& lhs, auto&& rhs) -> bool {
        using T = std::decay_t<decltype(lhs)>;
        using U = std::decay_t<decltype(rhs)>;
        if constexpr (std::is_same_v<T, U>) {
            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                if (op == "=")  return true;
                if (op == "!=") return false;
                return false;
            } else {
                if (op == "=")  return lhs == rhs;
                if (op == "!=") return lhs != rhs;
                if (op == "<")  return lhs <  rhs;
                if (op == ">")  return lhs >  rhs;
                if (op == "<=") return lhs <= rhs;
                if (op == ">=") return lhs >= rhs;
            }
        }
        return false;
    }, cell, value);
}
