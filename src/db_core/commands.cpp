#include "db_core/commands.h"
#include <algorithm>

CreateDatabaseCommand::CreateDatabaseCommand(std::string n) : name_(std::move(n)) {}

QueryResult CreateDatabaseCommand::execute(DatabaseManager& mgr, Session& session) {
    mgr.createDatabase(name_, session);
    return {false, true, {}, {}, {}, 0, "Database '" + name_ + "' created.", false};
}

DropDatabaseCommand::DropDatabaseCommand(std::string n) : name_(std::move(n)) {}

QueryResult DropDatabaseCommand::execute(DatabaseManager& mgr, Session& session) {
    mgr.dropDatabase(name_, session);
    return {false, true, {}, {}, {}, 0, "Database '" + name_ + "' dropped.", false};
}

UseDatabaseCommand::UseDatabaseCommand(std::string n) : name_(std::move(n)) {}

QueryResult UseDatabaseCommand::execute(DatabaseManager& mgr, Session& session) {
    mgr.useDatabase(name_, session);
    return {false, true, {}, {}, {}, 0, "Switched to database '" + name_ + "'.", false};
}

CreateTableCommand::CreateTableCommand(std::string n, std::vector<ColumnSchema> cols)
    : name_(std::move(n)), columns_(std::move(cols)) {}

QueryResult CreateTableCommand::execute(DatabaseManager& mgr, Session& session) {
    mgr.getCurrentDatabase(session).createTable(name_, TableSchema{columns_});
    return {false, true, {}, {}, {}, 0, "Table '" + name_ + "' created.", false};
}

DropTableCommand::DropTableCommand(std::string n) : name_(std::move(n)) {}

QueryResult DropTableCommand::execute(DatabaseManager& mgr, Session& session) {
    mgr.getCurrentDatabase(session).dropTable(name_);
    return {false, true, {}, {}, {}, 0, "Table '" + name_ + "' dropped.", false};
}

InsertCommand::InsertCommand(std::string t, std::vector<std::string> targetCols, std::vector<std::vector<Value>> allRowsVals)
    : table_(std::move(t)), targetColumns_(std::move(targetCols)), allRowsValues_(std::move(allRowsVals)) {}

QueryResult InsertCommand::execute(DatabaseManager& mgr, Session& session) {
    auto& table = mgr.getCurrentDatabase(session).getTable(table_);
    size_t insertedCount = 0;

    for (const auto& rowVals : allRowsValues_) {
        if (!targetColumns_.empty()) {
            Row alignedRow(table.schema.columns.size(), nullptr);
            for (size_t i = 0; i < targetColumns_.size(); ++i) {
                for (size_t j = 0; j < table.schema.columns.size(); ++j) {
                    if (table.schema.columns[j].name == targetColumns_[i]) {
                        alignedRow[j] = rowVals[i];
                        break;
                    }
                }
            }
            table.insert(alignedRow);
        } else {
            table.insert(rowVals);
        }
        insertedCount++;
    }

    return {false, false, {}, {}, {}, insertedCount, "", false};
}

SelectCommand::SelectCommand(std::string t, std::vector<std::string> p, std::shared_ptr<ConditionNode> c,
                             std::optional<OrderBy> orderBy, std::optional<size_t> limit, std::optional<size_t> offset)
    : table_(std::move(t)), projection_(std::move(p)), condition_(std::move(c))
    , orderBy_(std::move(orderBy)), limit_(limit), offset_(offset) {}

static int valueCompare(const Value& a, const Value& b) {
    return std::visit([](auto&& lhs, auto&& rhs) -> int {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;
        if constexpr (std::is_same_v<L, R> && !std::is_same_v<L, std::nullptr_t>) {
            if (lhs < rhs) return -1;
            if (lhs > rhs) return  1;
            return 0;
        }
        return 0;
    }, a, b);
}

QueryResult SelectCommand::execute(DatabaseManager& mgr, Session& session) {
    auto& t = mgr.getCurrentDatabase(session).getTable(table_);

    auto rows = t.selectFiltered(condition_.get());

    if (orderBy_) {
        size_t colIdx = t.schema.getColumnIndex(orderBy_->column);
        bool asc = orderBy_->ascending;
        std::stable_sort(rows.begin(), rows.end(), [colIdx, asc](const Row& a, const Row& b) {
            int cmp = valueCompare(a[colIdx], b[colIdx]);
            return asc ? cmp < 0 : cmp > 0;
        });
    }

    size_t start = offset_.value_or(0);
    if (start >= rows.size()) rows.clear();
    else rows.erase(rows.begin(), rows.begin() + static_cast<std::ptrdiff_t>(start));

    if (limit_ && rows.size() > *limit_)
        rows.resize(*limit_);

    QueryResult res;
    res.isSelect = true;

    std::vector<size_t> indices;
    if (projection_.size() == 1 && projection_[0] == "*") {
        for (size_t i = 0; i < t.schema.columns.size(); ++i) indices.push_back(i);
        for (const auto& col : t.schema.columns) res.columns.push_back(col.name);
    } else {
        for (const auto& col : projection_) {
            indices.push_back(t.schema.getColumnIndex(col));
            res.columns.push_back(col);
        }
    }

    for (const auto& row : rows) {
        Row projected;
        for (size_t idx : indices) projected.push_back(row[idx]);
        res.rows.push_back(projected);
    }

    res.affectedRows = res.rows.size();
    return res;
}

UpdateCommand::UpdateCommand(std::string t, std::vector<std::pair<std::string, Value>> assigns, std::shared_ptr<ConditionNode> cond)
    : table_(std::move(t)), assignments_(std::move(assigns)), condition_(std::move(cond)) {}

QueryResult UpdateCommand::execute(DatabaseManager& mgr, Session& session) {
    size_t count = mgr.getCurrentDatabase(session).getTable(table_).update(assignments_, condition_.get());
    return {false, false, {}, {}, {}, count, "", false};
}

DeleteCommand::DeleteCommand(std::string t, std::shared_ptr<ConditionNode> cond)
    : table_(std::move(t)), condition_(std::move(cond)) {}

QueryResult DeleteCommand::execute(DatabaseManager& mgr, Session& session) {
    size_t count = mgr.getCurrentDatabase(session).getTable(table_).remove(condition_.get());
    return {false, false, {}, {}, {}, count, "", false};
}

QueryResult BeginCommand::execute(DatabaseManager& mgr, Session& session) {
    mgr.beginTransaction(session);
    return {false, true, {}, {}, {}, 0, "Transaction started.", false};
}

QueryResult CommitCommand::execute(DatabaseManager& mgr, Session& session) {
    mgr.commitTransaction(session);
    return {false, true, {}, {}, {}, 0, "Transaction committed.", false};
}

QueryResult RollbackCommand::execute(DatabaseManager& mgr, Session& session) {
    mgr.rollbackTransaction(session);
    return {false, true, {}, {}, {}, 0, "Transaction rolled back.", false};
}