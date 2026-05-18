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

SelectCommand::SelectCommand(std::string t, std::vector<SelectExpr> exprs, std::shared_ptr<ConditionNode> c,
                             std::optional<std::string> groupBy,
                             std::optional<OrderBy> orderBy, std::optional<size_t> limit, std::optional<size_t> offset)
    : table_(std::move(t)), exprs_(std::move(exprs)), condition_(std::move(c))
    , groupBy_(std::move(groupBy)), orderBy_(std::move(orderBy)), limit_(limit), offset_(offset) {}

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

static Value computeAggregate(AggFunc func, const std::string& col,
                               const std::vector<Row>& rows, const TableSchema& schema) {
    if (func == AggFunc::COUNT) return static_cast<int>(rows.size());
    if (rows.empty()) return nullptr;

    size_t idx = schema.getColumnIndex(col);

    if (func == AggFunc::MIN) {
        Value best = rows[0][idx];
        for (size_t i = 1; i < rows.size(); ++i)
            if (valueCompare(rows[i][idx], best) < 0) best = rows[i][idx];
        return best;
    }
    if (func == AggFunc::MAX) {
        Value best = rows[0][idx];
        for (size_t i = 1; i < rows.size(); ++i)
            if (valueCompare(rows[i][idx], best) > 0) best = rows[i][idx];
        return best;
    }

    bool hasFloat = false;
    double total = 0;
    for (const auto& row : rows) {
        std::visit([&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int>)        total += v;
            else if constexpr (std::is_same_v<T, float>) { total += v; hasFloat = true; }
        }, row[idx]);
    }
    if (func == AggFunc::AVG) return static_cast<float>(total / static_cast<double>(rows.size()));
    return hasFloat ? Value(static_cast<float>(total)) : Value(static_cast<int>(total));
}

static Row buildResultRow(const std::vector<SelectExpr>& exprs,
                           const std::vector<Row>& groupRows, const TableSchema& schema) {
    Row result;
    for (const auto& expr : exprs) {
        if (expr.isAggregate)
            result.push_back(computeAggregate(expr.func, expr.column, groupRows, schema));
        else {
            size_t idx = schema.getColumnIndex(expr.column);
            result.push_back(groupRows.empty() ? Value(nullptr) : groupRows[0][idx]);
        }
    }
    return result;
}

static void applyOrderOffsetLimit(std::vector<Row>& rows,
                                   const std::optional<OrderBy>& orderBy,
                                   const std::vector<std::string>& cols,
                                   std::optional<size_t> offset, std::optional<size_t> limit) {
    if (orderBy) {
        size_t sortIdx = 0;
        for (size_t i = 0; i < cols.size(); ++i)
            if (cols[i] == orderBy->column) { sortIdx = i; break; }
        bool asc = orderBy->ascending;
        std::stable_sort(rows.begin(), rows.end(), [sortIdx, asc](const Row& a, const Row& b) {
            int cmp = valueCompare(a[sortIdx], b[sortIdx]);
            return asc ? cmp < 0 : cmp > 0;
        });
    }
    size_t start = offset.value_or(0);
    if (start >= rows.size()) rows.clear();
    else rows.erase(rows.begin(), rows.begin() + static_cast<std::ptrdiff_t>(start));
    if (limit && rows.size() > *limit) rows.resize(*limit);
}

QueryResult SelectCommand::execute(DatabaseManager& mgr, Session& session) {
    auto& t = mgr.getCurrentDatabase(session).getTable(table_);
    auto filteredRows = t.selectFiltered(condition_.get());

    bool isStar  = exprs_.size() == 1 && !exprs_[0].isAggregate && exprs_[0].column == "*";
    bool hasAgg  = std::any_of(exprs_.begin(), exprs_.end(), [](const SelectExpr& e) { return e.isAggregate; });

    QueryResult res;
    res.isSelect = true;
    if (isStar)
        for (const auto& col : t.schema.columns) res.columns.push_back(col.name);
    else
        for (const auto& expr : exprs_) res.columns.push_back(expr.alias);

    if (!hasAgg && !groupBy_) {
        if (orderBy_) {
            size_t colIdx = t.schema.getColumnIndex(orderBy_->column);
            bool asc = orderBy_->ascending;
            // stable_sort сохраняет порядок вставки для равных значений,
        // что делает вывод детерминированным при одинаковых ключах сортировки.
        std::stable_sort(filteredRows.begin(), filteredRows.end(),
                [colIdx, asc](const Row& a, const Row& b) {
                    int cmp = valueCompare(a[colIdx], b[colIdx]);
                    return asc ? cmp < 0 : cmp > 0;
                });
        }
        size_t start = offset_.value_or(0);
        if (start >= filteredRows.size()) filteredRows.clear();
        else filteredRows.erase(filteredRows.begin(), filteredRows.begin() + static_cast<std::ptrdiff_t>(start));
        if (limit_ && filteredRows.size() > *limit_) filteredRows.resize(*limit_);

        std::vector<size_t> indices;
        if (isStar)
            for (size_t i = 0; i < t.schema.columns.size(); ++i) indices.push_back(i);
        else
            for (const auto& expr : exprs_) indices.push_back(t.schema.getColumnIndex(expr.column));

        for (const auto& row : filteredRows) {
            Row projected;
            for (size_t idx : indices) projected.push_back(row[idx]);
            res.rows.push_back(projected);
        }
    } else {
        if (groupBy_) {
            size_t groupColIdx = t.schema.getColumnIndex(*groupBy_);
            std::vector<Value> groupOrder;
            std::vector<std::vector<Row>> groupData;
            for (const auto& row : filteredRows) {
                const Value& key = row[groupColIdx];
                bool found = false;
                for (size_t i = 0; i < groupOrder.size(); ++i) {
                    if (valueCompare(groupOrder[i], key) == 0) {
                        groupData[i].push_back(row); found = true; break;
                    }
                }
                if (!found) { groupOrder.push_back(key); groupData.push_back({row}); }
            }
            for (size_t i = 0; i < groupOrder.size(); ++i)
                res.rows.push_back(buildResultRow(exprs_, groupData[i], t.schema));
        } else {
            res.rows.push_back(buildResultRow(exprs_, filteredRows, t.schema));
        }
        applyOrderOffsetLimit(res.rows, orderBy_, res.columns, offset_, limit_);
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

ExportCommand::ExportCommand(std::string tableName, std::string filePath)
    : tableName_(std::move(tableName)), filePath_(std::move(filePath)) {}

QueryResult ExportCommand::execute(DatabaseManager& mgr, Session& session) {
    // 1. Получаем текущую БД
    auto& db = mgr.getCurrentDatabase(session);

    // 2. Вызываем логику экспорта.
    // Пример, если метод реализован в Database и принимает имя таблицы и путь:
    db.exportTableToCsv(tableName_, filePath_);

    // Либо, если это делается напрямую через движок:
    // CsvStorageEngine::exportTable(db.getTable(tableName_), filePath_);

    return {false, true, {}, {}, {}, 0, "Table '" + tableName_ + "' successfully exported to " + filePath_, false};
}

ImportCommand::ImportCommand(std::string tableName, std::string filePath)
    : tableName_(std::move(tableName)), filePath_(std::move(filePath)) {}

QueryResult ImportCommand::execute(DatabaseManager& mgr, Session& session) {
    auto& db = mgr.getCurrentDatabase(session);

    // Вызываем логику импорта
    db.importTableFromCsv(tableName_, filePath_);

    return {false, true, {}, {}, {}, 0, "Table '" + tableName_ + "' successfully imported from " + filePath_, false};
}
