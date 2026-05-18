#pragma once
#include "database.h"
#include "storage_engine.h"
#include "logger.h"
#include "session.h"
#include <memory>
#include <mutex>

class DatabaseManager {
    std::map<std::string, Database> databases_;
    std::unique_ptr<StorageEngine> storage_;
    std::unique_ptr<Logger> logger_;
    // Один глобальный мьютекс на весь менеджер: каждое TCP-соединение
    // обрабатывается в отдельном потоке, мьютекс гарантирует сериализацию.
    std::mutex mtx_;

    DatabaseManager() : logger_(std::make_unique<ConsoleLogger>()) {}
public:
    static DatabaseManager& instance() {
        static DatabaseManager inst;
        return inst;
    }

    void setStorageEngine(std::unique_ptr<StorageEngine> se) { storage_ = std::move(se); }
    void setLogger(std::unique_ptr<Logger> lg) { logger_ = std::move(lg); }
    Logger& logger() { return *logger_; }
    std::mutex& mutex() { return mtx_; }

    std::map<std::string, Database>& getDatabases() { return databases_; }
    const std::map<std::string, Database>& getDatabases() const { return databases_; }

    void createDatabase(const std::string& name, Session& session);
    void dropDatabase(const std::string& name, Session& session);
    void useDatabase(const std::string& name, Session& session);
    Database& getCurrentDatabase(Session& session);

    void beginTransaction(Session& session);
    void commitTransaction(Session& session);
    void rollbackTransaction(Session& session);

    void saveAll(const Session& session) {
        // Во время транзакции данные не сохраняются: COMMIT запишет финальное
        // состояние, ROLLBACK восстановит снимок без каких-либо записей на диск.
        if (session.inTransaction) return;
        if (storage_) storage_->saveAll(*this);
    }
    void loadAll() { if (storage_) storage_->loadAll(*this); }
};