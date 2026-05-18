#pragma once
#include "database.h"

class DatabaseManager;

class StorageEngine {
public:
    virtual ~StorageEngine() = default;
    virtual void saveDatabase(const Database& db, const std::string& path) = 0;
    virtual Database loadDatabase(const std::string& path) = 0;
    virtual void saveAll(const DatabaseManager& mgr) = 0;
    virtual void loadAll(DatabaseManager& mgr) = 0;
};


class JsonFileStorageEngine : public StorageEngine {
public:
    void saveDatabase(const Database& db, const std::string& path) override;
    Database loadDatabase(const std::string& path) override;
    void saveAll(const DatabaseManager& mgr) override;
    void loadAll(DatabaseManager& mgr) override;
};


class CsvStorageEngine {
public:
    static void exportDatabase(const Database& db, const std::string& basePath);

    static void exportAll(const DatabaseManager& mgr, const std::string& basePath = "./data_csv");

    static void importDatabase(Database& db, const std::string& basePath);


    static void importAll(DatabaseManager& mgr, const std::string& basePath = "./data_csv");
};
