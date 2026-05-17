#pragma once
#include "types.h"
#include <memory>

class ConditionNode {
public:
    virtual ~ConditionNode() = default;
    virtual bool evaluate(const Row& row, const TableSchema& schema) const = 0;
};

class ComparisonNode : public ConditionNode {
    std::string column;
    std::string op;
    Value value;
public:
    ComparisonNode(std::string col, std::string o, Value val)
        : column(std::move(col)), op(std::move(o)), value(std::move(val)) {}

    bool evaluate(const Row& row, const TableSchema& schema) const override;
};

class AndNode : public ConditionNode {
    std::shared_ptr<ConditionNode> left, right;
public:
    AndNode(std::shared_ptr<ConditionNode> l, std::shared_ptr<ConditionNode> r)
        : left(std::move(l)), right(std::move(r)) {}

    bool evaluate(const Row& row, const TableSchema& schema) const override {
        if (!left || !right) return false;
        return left->evaluate(row, schema) && right->evaluate(row, schema);
    }
};

class OrNode : public ConditionNode {
    std::shared_ptr<ConditionNode> left, right;
public:
    OrNode(std::shared_ptr<ConditionNode> l, std::shared_ptr<ConditionNode> r)
        : left(std::move(l)), right(std::move(r)) {}

    bool evaluate(const Row& row, const TableSchema& schema) const override {
        if (!left || !right) return false;
        return left->evaluate(row, schema) || right->evaluate(row, schema);
    }
};

class NotNode : public ConditionNode {
    std::shared_ptr<ConditionNode> child;
public:
    NotNode(std::shared_ptr<ConditionNode> c) : child(std::move(c)) {}

    bool evaluate(const Row& row, const TableSchema& schema) const override {
        if (!child) return false;
        return !child->evaluate(row, schema);
    }
};
