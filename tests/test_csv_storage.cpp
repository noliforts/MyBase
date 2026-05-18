#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "db_core/csv_storage_engine.h"
#include "db_core/table.h"
#include "db_core/types.h"

namespace fs = std::filesystem;

class CsvStorageTest : public ::testing::Test {
protected:
    std::string test_file_path;

    void SetUp() override {
        test_file_path = (fs::temp_directory_path() / "mybase_test_data.csv").string();
    }

    void TearDown() override {
        if (fs::exists(test_file_path)) {
            fs::remove(test_file_path);
        }
    }

    Table createMockTable() {
        TableSchema schema;
        schema.columns = {
            {"id", DataType::INT},
            {"name", DataType::TEXT},
            {"price", DataType::FLOAT},
            {"active", DataType::BOOL}
        };
        return Table(schema);
    }
};

TEST_F(CsvStorageTest, ExportTableSuccessfully) {
    Table table = createMockTable();

    Row row1 = {1, std::string("Widget"), 9.99f, true};
    Row row2 = {2, std::string("Gadget"), 49.50f, false};
    table.insert(row1);
    table.insert(row2);

    ASSERT_NO_THROW(CsvStorageEngine::exportTable(table, test_file_path));
    ASSERT_TRUE(fs::exists(test_file_path));

    std::ifstream file(test_file_path);
    std::string line;

    std::getline(file, line);
    EXPECT_EQ(line, "id,name,price,active");

    std::getline(file, line);
    EXPECT_EQ(line, "1,\"Widget\",9.990000,true");
}

TEST_F(CsvStorageTest, ImportTableSuccessfully) {
    std::ofstream file(test_file_path);
    file << "id,name,price,active\n";
    file << "10,\"MacBook\",1499.90,true\n";
    file << "20,\"Mouse\",25.00,false\n";
    file.close();

    Table table = createMockTable();

    ASSERT_NO_THROW(CsvStorageEngine::importTable(table, test_file_path));

    ASSERT_EQ(table.rows.size(), 2);

    EXPECT_EQ(std::get<int>(table.rows[0][0]), 10);
    EXPECT_EQ(std::get<std::string>(table.rows[0][1]), "MacBook");
    EXPECT_NEAR(std::get<float>(table.rows[0][2]), 1499.90f, 0.01);
    EXPECT_EQ(std::get<bool>(table.rows[0][3]), true);
}

TEST_F(CsvStorageTest, HandleCommaInsideQuotes) {
    std::ofstream file(test_file_path);
    file << "id,name,price,active\n";
    file << "1,\"Соль, перец и сахар\",5.50,true\n";
    file.close();

    Table table = createMockTable();
    ASSERT_NO_THROW(CsvStorageEngine::importTable(table, test_file_path));

    ASSERT_EQ(table.rows.size(), 1);
    EXPECT_EQ(std::get<std::string>(table.rows[0][1]), "Соль, перец и сахар");
    EXPECT_NEAR(std::get<float>(table.rows[0][2]), 5.50f, 0.01);
}

TEST_F(CsvStorageTest, ImportMismatchedColumnCountThrows) {
    std::ofstream file(test_file_path);
    file << "id,name,price,active\n";
    file << "1,\"Broken Row\",9.99\n";
    file.close();

    Table table = createMockTable();

    EXPECT_THROW(CsvStorageEngine::importTable(table, test_file_path), std::runtime_error);
}

TEST_F(CsvStorageTest, ImportNonExistentFileThrows) {
    Table table = createMockTable();
    std::string fake_path = "this_file_definitely_does_not_exist.csv";

    EXPECT_THROW(CsvStorageEngine::importTable(table, fake_path), std::runtime_error);
}
