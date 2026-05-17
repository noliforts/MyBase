#include "db_core/table.h"
#include <algorithm>

void Table::insert(const Row& values) {
    if (values.size() != schema.columnCount()) {
        throw std::runtime_error("Insert error: Column count mismatch.");
    }
    rows.push_back(values);
}

std::vector<Row> Table::select(const std::vector<std::string>& projection, const ConditionNode* cond) const {
    std::vector<Row> result;
    std::vector<size_t> indices;

    if (projection.size() == 1 && projection[0] == "*") {
        for (size_t i = 0; i < schema.columnCount(); ++i) indices.push_back(i);
    } else {
        for (const auto& col : projection) indices.push_back(schema.getColumnIndex(col));
    }

    for (const auto& row : rows) {
        if (!cond || cond->evaluate(row, schema)) {
            Row projectedRow;
            for (size_t idx : indices) projectedRow.push_back(row[idx]);
            result.push_back(projectedRow);
        }
    }
    return result;
}

int Table::update(const std::vector<std::pair<std::string, Value>>& assignments, const ConditionNode* cond) {
    int count = 0;
    for (auto& row : rows) {
        if (!cond || cond->evaluate(row, schema)) {
            for (const auto& [col, val] : assignments) {
                row[schema.getColumnIndex(col)] = val;
            }
            count++;
        }
    }
    return count;
}

int Table::remove(const ConditionNode* cond) {
    size_t initialSize = rows.size();
    auto it = std::remove_if(rows.begin(), rows.end(), [this, cond](const Row& row) {
        return !cond || cond->evaluate(row, schema);
    });
    rows.erase(it, rows.end());
    return initialSize - rows.size();
}
