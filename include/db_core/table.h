#pragma once
#include "types.h"
#include "condition.h"

class Table {
public:
    TableSchema schema;
    std::vector<Row> rows;

    Table() = default;
    explicit Table(TableSchema s) : schema(std::move(s)) {}

    void insert(const Row& values);
    std::vector<Row> select(const std::vector<std::string>& projection, const ConditionNode* cond) const;
    int update(const std::vector<std::pair<std::string, Value>>& assignments, const ConditionNode* cond);
    int remove(const ConditionNode* cond);
};
