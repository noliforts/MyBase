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
