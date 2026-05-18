#pragma once
#include "condition.h"
#include "database_manager.h"
#include "types.h"
#include <memory>
#include <string>
#include <vector>
#include <utility>

class Command {
public:
    virtual ~Command() = default;
    virtual QueryResult execute(DatabaseManager& mgr) = 0;
};

class CreateDatabaseCommand : public Command {
    std::string name_;
public:
    explicit CreateDatabaseCommand(std::string n);
    QueryResult execute(DatabaseManager& mgr) override;
};

class UseDatabaseCommand : public Command {
    std::string name_;
public:
    explicit UseDatabaseCommand(std::string n);
    QueryResult execute(DatabaseManager& mgr) override;
};

class DropDatabaseCommand : public Command {
    std::string name_;
public:
    explicit DropDatabaseCommand(std::string n);
    QueryResult execute(DatabaseManager& mgr) override;
};

class CreateTableCommand : public Command {
    std::string name_;
    std::vector<ColumnSchema> columns_;
public:
    CreateTableCommand(std::string n, std::vector<ColumnSchema> cols);
    QueryResult execute(DatabaseManager& mgr) override;
};

class DropTableCommand : public Command {
    std::string name_;
public:
    explicit DropTableCommand(std::string n);
    QueryResult execute(DatabaseManager& mgr) override;
};

class InsertCommand : public Command {
    std::string table_;
    std::vector<std::string> targetColumns_;
    std::vector<std::vector<Value>> allRowsValues_;
public:
    InsertCommand(std::string t, std::vector<std::string> targetCols, std::vector<std::vector<Value>> allRowsVals);
    QueryResult execute(DatabaseManager& mgr) override;
};

class SelectCommand : public Command {
    std::string table_;
    std::vector<std::string> projection_;
    std::shared_ptr<ConditionNode> condition_;
public:
    SelectCommand(std::string t, std::vector<std::string> p, std::shared_ptr<ConditionNode> c);
    QueryResult execute(DatabaseManager& mgr) override;
};

class UpdateCommand : public Command {
    std::string table_;
    std::vector<std::pair<std::string, Value>> assignments_;
    std::shared_ptr<ConditionNode> condition_;
public:
    UpdateCommand(std::string t, std::vector<std::pair<std::string, Value>> assigns, std::shared_ptr<ConditionNode> cond);
    QueryResult execute(DatabaseManager& mgr) override;
};

class DeleteCommand : public Command {
    std::string table_;
    std::shared_ptr<ConditionNode> condition_;
public:
    DeleteCommand(std::string t, std::shared_ptr<ConditionNode> cond);
    QueryResult execute(DatabaseManager& mgr) override;
};

class BeginCommand : public Command {
public:
    QueryResult execute(DatabaseManager& mgr) override;
};

class CommitCommand : public Command {
public:
    QueryResult execute(DatabaseManager& mgr) override;
};

class RollbackCommand : public Command {
public:
    QueryResult execute(DatabaseManager& mgr) override;
};
