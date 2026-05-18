#pragma once
#include "condition.h"
#include "database_manager.h"
#include "session.h"
#include "types.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <utility>

struct OrderBy {
    std::string column;
    bool ascending = true;
};

enum class AggFunc { COUNT, SUM, MIN, MAX, AVG };

struct SelectExpr {
    bool isAggregate = false;
    std::string column;
    AggFunc func = AggFunc::COUNT;
    std::string alias;
};

class Command {
public:
    virtual ~Command() = default;
    virtual QueryResult execute(DatabaseManager& mgr, Session& session) = 0;
};

class CreateDatabaseCommand : public Command {
    std::string name_;
public:
    explicit CreateDatabaseCommand(std::string n);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class UseDatabaseCommand : public Command {
    std::string name_;
public:
    explicit UseDatabaseCommand(std::string n);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class DropDatabaseCommand : public Command {
    std::string name_;
public:
    explicit DropDatabaseCommand(std::string n);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class CreateTableCommand : public Command {
    std::string name_;
    std::vector<ColumnSchema> columns_;
public:
    CreateTableCommand(std::string n, std::vector<ColumnSchema> cols);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class DropTableCommand : public Command {
    std::string name_;
public:
    explicit DropTableCommand(std::string n);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class InsertCommand : public Command {
    std::string table_;
    std::vector<std::string> targetColumns_;
    std::vector<std::vector<Value>> allRowsValues_;
public:
    InsertCommand(std::string t, std::vector<std::string> targetCols, std::vector<std::vector<Value>> allRowsVals);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class SelectCommand : public Command {
    std::string table_;
    std::vector<SelectExpr> exprs_;
    std::shared_ptr<ConditionNode> condition_;
    std::optional<std::string> groupBy_;
    std::optional<OrderBy> orderBy_;
    std::optional<size_t> limit_;
    std::optional<size_t> offset_;
public:
    SelectCommand(std::string t, std::vector<SelectExpr> exprs, std::shared_ptr<ConditionNode> c,
                  std::optional<std::string> groupBy = std::nullopt,
                  std::optional<OrderBy> orderBy = std::nullopt,
                  std::optional<size_t> limit = std::nullopt,
                  std::optional<size_t> offset = std::nullopt);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class UpdateCommand : public Command {
    std::string table_;
    std::vector<std::pair<std::string, Value>> assignments_;
    std::shared_ptr<ConditionNode> condition_;
public:
    UpdateCommand(std::string t, std::vector<std::pair<std::string, Value>> assigns, std::shared_ptr<ConditionNode> cond);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class DeleteCommand : public Command {
    std::string table_;
    std::shared_ptr<ConditionNode> condition_;
public:
    DeleteCommand(std::string t, std::shared_ptr<ConditionNode> cond);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class BeginCommand : public Command {
public:
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class CommitCommand : public Command {
public:
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};

class RollbackCommand : public Command {
public:
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
};
class ExportCommand : public Command {
public:
    ExportCommand(std::string tableName, std::string filePath);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
private:
    std::string tableName_;
    std::string filePath_;
};
class ImportCommand : public Command {
public:
    ImportCommand(std::string tableName, std::string filePath);
    QueryResult execute(DatabaseManager& mgr, Session& session) override;
private:
    std::string tableName_;
    std::string filePath_;
};
