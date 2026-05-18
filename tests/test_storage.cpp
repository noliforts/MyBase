#include <gtest/gtest.h>
#include "db_core/database_manager.h"
#include <filesystem>
#include <fstream>

TEST(StorageTest, SaveAndLoadIdentity) {
    {
        DatabaseManager& mgr = DatabaseManager::instance();
        mgr.createDatabase("shop");
        mgr.useDatabase("shop");

        TableSchema schema;
        schema.columns = {{"id", DataType::INT}, {"title", DataType::TEXT}};

        mgr.getCurrentDatabase().createTable("products", schema);
        mgr.getCurrentDatabase().getTable("products").insert({101, std::string("Phone")});

        mgr.saveAll();
    }

    {
        DatabaseManager& mgr2 = DatabaseManager::instance();
        mgr2.loadAll();

        mgr2.useDatabase("shop");
        auto& table = mgr2.getCurrentDatabase().getTable("products");

        EXPECT_EQ(table.schema.columns[0].name, "id");
        EXPECT_EQ(table.schema.columns[1].name, "title");

        auto rows = table.select({"*"}, nullptr);
        ASSERT_EQ(rows.size(), 1);
        EXPECT_EQ(std::get<int>(rows[0][0]), 101);
        EXPECT_EQ(std::get<std::string>(rows[0][1]), "Phone");
    }
}

TEST(StorageTest, TransactionRollback) {
    DatabaseManager& mgr = DatabaseManager::instance();
    mgr.createDatabase("txdb");
    mgr.useDatabase("txdb");
    mgr.getCurrentDatabase().createTable("t", TableSchema{{{"x", DataType::INT}}});
    mgr.getCurrentDatabase().getTable("t").insert({42});

    mgr.beginTransaction();
    mgr.getCurrentDatabase().getTable("t").insert({99});
    EXPECT_EQ(mgr.getCurrentDatabase().getTable("t").select({"*"}, nullptr).size(), 2u);

    mgr.rollbackTransaction();
    EXPECT_EQ(mgr.getCurrentDatabase().getTable("t").select({"*"}, nullptr).size(), 1u);
    EXPECT_EQ(std::get<int>(mgr.getCurrentDatabase().getTable("t").select({"*"}, nullptr)[0][0]), 42);
}

TEST(StorageTest, TransactionCommit) {
    DatabaseManager& mgr = DatabaseManager::instance();
    mgr.useDatabase("txdb");

    mgr.beginTransaction();
    mgr.getCurrentDatabase().getTable("t").insert({7});
    mgr.commitTransaction();

    EXPECT_FALSE(mgr.isInTransaction());
    EXPECT_EQ(mgr.getCurrentDatabase().getTable("t").select({"*"}, nullptr).size(), 2u);
}

TEST(StorageTest, NestedTransactionThrows) {
    DatabaseManager& mgr = DatabaseManager::instance();
    mgr.beginTransaction();
    EXPECT_THROW(mgr.beginTransaction(), std::runtime_error);
    mgr.rollbackTransaction();

    EXPECT_THROW(mgr.commitTransaction(), std::runtime_error);
    EXPECT_THROW(mgr.rollbackTransaction(), std::runtime_error);
}

TEST(StorageTest, CorruptedFileHandling) {
    std::string testPath = "./data";
    std::filesystem::create_directories(testPath);

    std::ofstream corruptFile(testPath + "/corrupt_table.jsonl");
    corruptFile << "{ invalid json: [ } \n";
    corruptFile.close();

    DatabaseManager& mgr = DatabaseManager::instance();
    EXPECT_NO_THROW(mgr.loadAll());

    std::filesystem::remove(testPath + "/corrupt_table.jsonl");
}
