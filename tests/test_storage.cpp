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

TEST(StorageTest, CorruptedFileHandling) {
    std::string testPath = "./data";
    std::filesystem::create_directories(testPath);

    std::ofstream corruptFile(testPath + "/corrupt_table.jsonl");
    corruptFile << "{ invalid json: [ } \n";
    corruptFile.close();

    DatabaseManager& mgr = DatabaseManager::instance();
    EXPECT_THROW({
        mgr.loadAll();
    }, std::runtime_error);
}
