#include <gtest/gtest.h>
#include "db_core/database_manager.h"
#include <filesystem>
#include <fstream>

static Session makeSession(const std::string& db = "") {
    Session s;
    s.currentDb = db;
    return s;
}

TEST(StorageTest, SaveAndLoadIdentity) {
    {
        DatabaseManager& mgr = DatabaseManager::instance();
        Session s;
        mgr.createDatabase("shop", s);
        mgr.useDatabase("shop", s);

        TableSchema schema;
        schema.columns = {{"id", DataType::INT}, {"title", DataType::TEXT}};

        mgr.getCurrentDatabase(s).createTable("products", schema);
        mgr.getCurrentDatabase(s).getTable("products").insert({101, std::string("Phone")});

        mgr.saveAll(s);
    }

    {
        DatabaseManager& mgr2 = DatabaseManager::instance();
        Session s;
        mgr2.loadAll();

        mgr2.useDatabase("shop", s);
        auto& table = mgr2.getCurrentDatabase(s).getTable("products");

        EXPECT_EQ(table.schema.columns[0].name, "id");
        EXPECT_EQ(table.schema.columns[1].name, "title");

        auto rows = table.select({"*"}, nullptr);
        ASSERT_EQ(rows.size(), 1u);
        EXPECT_EQ(std::get<int>(rows[0][0]), 101);
        EXPECT_EQ(std::get<std::string>(rows[0][1]), "Phone");
    }
}

TEST(StorageTest, TransactionRollback) {
    DatabaseManager& mgr = DatabaseManager::instance();
    Session s = makeSession();
    mgr.createDatabase("txdb", s);
    mgr.useDatabase("txdb", s);
    mgr.getCurrentDatabase(s).createTable("t", TableSchema{{{"x", DataType::INT}}});
    mgr.getCurrentDatabase(s).getTable("t").insert({42});

    mgr.beginTransaction(s);
    mgr.getCurrentDatabase(s).getTable("t").insert({99});
    EXPECT_EQ(mgr.getCurrentDatabase(s).getTable("t").select({"*"}, nullptr).size(), 2u);

    mgr.rollbackTransaction(s);
    EXPECT_EQ(mgr.getCurrentDatabase(s).getTable("t").select({"*"}, nullptr).size(), 1u);
    EXPECT_EQ(std::get<int>(mgr.getCurrentDatabase(s).getTable("t").select({"*"}, nullptr)[0][0]), 42);
}

TEST(StorageTest, TransactionCommit) {
    DatabaseManager& mgr = DatabaseManager::instance();
    Session s = makeSession("txdb");

    mgr.beginTransaction(s);
    mgr.getCurrentDatabase(s).getTable("t").insert({7});
    mgr.commitTransaction(s);

    EXPECT_FALSE(s.inTransaction);
    EXPECT_EQ(mgr.getCurrentDatabase(s).getTable("t").select({"*"}, nullptr).size(), 2u);
}

TEST(StorageTest, NestedTransactionThrows) {
    DatabaseManager& mgr = DatabaseManager::instance();
    Session s;
    mgr.beginTransaction(s);
    EXPECT_THROW(mgr.beginTransaction(s), std::runtime_error);
    mgr.rollbackTransaction(s);

    EXPECT_THROW(mgr.commitTransaction(s), std::runtime_error);
    EXPECT_THROW(mgr.rollbackTransaction(s), std::runtime_error);
}

TEST(StorageTest, CorruptedFileHandling) {
    std::string testPath = "./data";
    std::filesystem::create_directories(testPath);

    std::ofstream corruptFile(testPath + "/corrupt_table.jsonl");
    corruptFile << "{ invalid json: [ } \n";
    corruptFile.close();

    DatabaseManager& mgr = DatabaseManager::instance();
    Session s;
    EXPECT_NO_THROW(mgr.loadAll());

    std::filesystem::remove(testPath + "/corrupt_table.jsonl");
}
