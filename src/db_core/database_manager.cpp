#include "db_core/database_manager.h"

void DatabaseManager::createDatabase(const std::string& name) {
    if (databases_.count(name)) throw std::runtime_error("Database already exists");
    databases_[name] = Database(name);
    logger_->log("Created database: " + name);
}

void DatabaseManager::dropDatabase(const std::string& name) {
    if (!databases_.erase(name)) throw std::runtime_error("Database not found");
    if (currentDb_ == name) currentDb_.clear();
    logger_->log("Dropped database: " + name);
}

void DatabaseManager::useDatabase(const std::string& name) {
    if (!databases_.count(name)) throw std::runtime_error("Database not found");
    currentDb_ = name;
    logger_->log("Switched to database: " + name);
}

Database& DatabaseManager::getCurrentDatabase() {
    if (currentDb_.empty()) throw std::runtime_error("No database selected");
    return databases_[currentDb_];
}

void DatabaseManager::beginTransaction() {
    if (inTransaction_) throw std::runtime_error("Transaction already in progress");
    snapshot_ = databases_;
    snapshotCurrentDb_ = currentDb_;
    inTransaction_ = true;
    logger_->log("Transaction started");
}

void DatabaseManager::commitTransaction() {
    if (!inTransaction_) throw std::runtime_error("No active transaction");
    snapshot_.reset();
    snapshotCurrentDb_.clear();
    inTransaction_ = false;
    logger_->log("Transaction committed");
}

void DatabaseManager::rollbackTransaction() {
    if (!inTransaction_) throw std::runtime_error("No active transaction");
    databases_ = std::move(*snapshot_);
    currentDb_ = snapshotCurrentDb_;
    snapshot_.reset();
    snapshotCurrentDb_.clear();
    inTransaction_ = false;
    logger_->log("Transaction rolled back");
}
