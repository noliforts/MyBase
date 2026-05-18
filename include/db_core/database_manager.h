#pragma once
#include "database.h"
#include "storage_engine.h"
#include "logger.h"
#include <memory>
#include <optional>

class DatabaseManager {
    std::map<std::string, Database> databases_;
    std::unique_ptr<StorageEngine> storage_;
    std::unique_ptr<Logger> logger_;
    std::string currentDb_;

    bool inTransaction_ = false;
    std::optional<std::map<std::string, Database>> snapshot_;
    std::string snapshotCurrentDb_;

    DatabaseManager() : logger_(std::make_unique<ConsoleLogger>()) {}
public:
    static DatabaseManager& instance() {
        static DatabaseManager inst;
        return inst;
    }

    void setStorageEngine(std::unique_ptr<StorageEngine> se) { storage_ = std::move(se); }
    void setLogger(std::unique_ptr<Logger> lg) { logger_ = std::move(lg); }
    Logger& logger() { return *logger_; }

    std::map<std::string, Database>& getDatabases() { return databases_; }
    const std::map<std::string, Database>& getDatabases() const { return databases_; }

    void createDatabase(const std::string& name);
    void dropDatabase(const std::string& name);
    void useDatabase(const std::string& name);

    Database& getCurrentDatabase();
    bool hasCurrentDatabase() const { return !currentDb_.empty(); }
    bool isInTransaction() const { return inTransaction_; }

    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();

    void saveAll() {
        if (inTransaction_) return;
        if (storage_) storage_->saveAll(*this);
    }
    void loadAll() { if (storage_) storage_->loadAll(*this); }
};
