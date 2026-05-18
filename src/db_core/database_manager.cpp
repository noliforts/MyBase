#include "db_core/database_manager.h"

void DatabaseManager::createDatabase(const std::string& name, Session& session) {
    if (databases_.count(name)) throw std::runtime_error("Database already exists");
    databases_[name] = Database(name);
    logger_->log("Created database: " + name);
}

void DatabaseManager::dropDatabase(const std::string& name, Session& session) {
    if (!databases_.erase(name)) throw std::runtime_error("Database not found");
    if (session.currentDb == name) session.currentDb.clear();
    logger_->log("Dropped database: " + name);
}

void DatabaseManager::useDatabase(const std::string& name, Session& session) {
    if (!databases_.count(name)) throw std::runtime_error("Database not found");
    session.currentDb = name;
    logger_->log("Switched to database: " + name);
}

Database& DatabaseManager::getCurrentDatabase(Session& session) {
    if (session.currentDb.empty()) throw std::runtime_error("No database selected");
    return databases_[session.currentDb];
}

void DatabaseManager::beginTransaction(Session& session) {
    if (session.inTransaction) throw std::runtime_error("Transaction already in progress");
    session.snapshot = databases_;
    session.snapshotCurrentDb = session.currentDb;
    session.inTransaction = true;
    logger_->log("Transaction started");
}

void DatabaseManager::commitTransaction(Session& session) {
    if (!session.inTransaction) throw std::runtime_error("No active transaction");
    session.snapshot.reset();
    session.snapshotCurrentDb.clear();
    session.inTransaction = false;
    logger_->log("Transaction committed");
}

void DatabaseManager::rollbackTransaction(Session& session) {
    if (!session.inTransaction) throw std::runtime_error("No active transaction");
    databases_ = std::move(*session.snapshot);
    session.currentDb = session.snapshotCurrentDb;
    session.snapshot.reset();
    session.snapshotCurrentDb.clear();
    session.inTransaction = false;
    logger_->log("Transaction rolled back");
}