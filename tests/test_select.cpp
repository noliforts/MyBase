#include <gtest/gtest.h>
#include "db_core/database_manager.h"
#include "db_core/lexer.h"
#include "db_core/parser.h"

class SelectTest : public ::testing::Test {
protected:
    DatabaseManager& mgr = DatabaseManager::instance();
    Session session;

    void SetUp() override {
        session = {};
        mgr.createDatabase("seldb", session);
        mgr.useDatabase("seldb", session);
        mgr.getCurrentDatabase(session).createTable(
            "scores",
            TableSchema{{{"name", DataType::TEXT}, {"score", DataType::INT}}}
        );
        auto& t = mgr.getCurrentDatabase(session).getTable("scores");
        t.insert({std::string("Charlie"), 70});
        t.insert({std::string("Alice"),   95});
        t.insert({std::string("Bob"),     80});
        t.insert({std::string("Diana"),   80});
        t.insert({std::string("Eve"),     60});
    }

    void TearDown() override {
        mgr.dropDatabase("seldb", session);
    }

    QueryResult run(const std::string& sql) {
        Lexer lexer(sql);
        Parser<Lexer> parser(lexer);
        auto cmd = parser.parse();
        return cmd->execute(mgr, session);
    }
};

TEST_F(SelectTest, OrderByAsc) {
    auto res = run("SELECT * FROM scores ORDER BY score ASC;");
    ASSERT_EQ(res.rows.size(), 5u);
    EXPECT_EQ(std::get<int>(res.rows[0][1]), 60);
    EXPECT_EQ(std::get<int>(res.rows[4][1]), 95);
}

TEST_F(SelectTest, OrderByDesc) {
    auto res = run("SELECT * FROM scores ORDER BY score DESC;");
    ASSERT_EQ(res.rows.size(), 5u);
    EXPECT_EQ(std::get<int>(res.rows[0][1]), 95);
    EXPECT_EQ(std::get<int>(res.rows[4][1]), 60);
}

TEST_F(SelectTest, OrderByDefaultIsAsc) {
    auto res = run("SELECT * FROM scores ORDER BY score;");
    EXPECT_EQ(std::get<int>(res.rows[0][1]), 60);
    EXPECT_EQ(std::get<int>(res.rows[4][1]), 95);
}

TEST_F(SelectTest, Limit) {
    auto res = run("SELECT * FROM scores LIMIT 2;");
    EXPECT_EQ(res.rows.size(), 2u);
}

TEST_F(SelectTest, Offset) {
    auto res = run("SELECT * FROM scores ORDER BY score ASC OFFSET 3;");
    ASSERT_EQ(res.rows.size(), 2u);
    EXPECT_EQ(std::get<int>(res.rows[0][1]), 80);
}

TEST_F(SelectTest, LimitAndOffset) {
    auto res = run("SELECT * FROM scores ORDER BY score ASC LIMIT 2 OFFSET 1;");
    ASSERT_EQ(res.rows.size(), 2u);
    EXPECT_EQ(std::get<int>(res.rows[0][1]), 70);
    EXPECT_EQ(std::get<int>(res.rows[1][1]), 80);
}

TEST_F(SelectTest, OrderByNonProjectedColumn) {
    auto res = run("SELECT name FROM scores ORDER BY score ASC;");
    ASSERT_EQ(res.rows.size(), 5u);
    EXPECT_EQ(res.columns.size(), 1u);
    EXPECT_EQ(std::get<std::string>(res.rows[0][0]), "Eve");
    EXPECT_EQ(std::get<std::string>(res.rows[4][0]), "Alice");
}

TEST_F(SelectTest, StableSortPreservesEqualOrder) {
    auto res = run("SELECT name FROM scores ORDER BY score ASC;");
    ASSERT_EQ(res.rows.size(), 5u);
    EXPECT_EQ(std::get<std::string>(res.rows[2][0]), "Bob");
    EXPECT_EQ(std::get<std::string>(res.rows[3][0]), "Diana");
}

TEST_F(SelectTest, OffsetBeyondSize) {
    auto res = run("SELECT * FROM scores OFFSET 100;");
    EXPECT_EQ(res.rows.size(), 0u);
}

TEST_F(SelectTest, CountStar) {
    auto res = run("SELECT COUNT(*) FROM scores;");
    ASSERT_EQ(res.rows.size(), 1u);
    EXPECT_EQ(std::get<int>(res.rows[0][0]), 5);
}

TEST_F(SelectTest, SumColumn) {
    auto res = run("SELECT SUM(score) FROM scores;");
    ASSERT_EQ(res.rows.size(), 1u);
    EXPECT_EQ(std::get<int>(res.rows[0][0]), 385);
}

TEST_F(SelectTest, AvgColumn) {
    auto res = run("SELECT AVG(score) FROM scores;");
    ASSERT_EQ(res.rows.size(), 1u);
    EXPECT_NEAR(std::get<float>(res.rows[0][0]), 77.0f, 0.01f);
}

TEST_F(SelectTest, MinColumn) {
    auto res = run("SELECT MIN(score) FROM scores;");
    ASSERT_EQ(res.rows.size(), 1u);
    EXPECT_EQ(std::get<int>(res.rows[0][0]), 60);
}

TEST_F(SelectTest, MaxColumn) {
    auto res = run("SELECT MAX(score) FROM scores;");
    ASSERT_EQ(res.rows.size(), 1u);
    EXPECT_EQ(std::get<int>(res.rows[0][0]), 95);
}

TEST_F(SelectTest, CountWithWhere) {
    auto res = run("SELECT COUNT(*) FROM scores WHERE score > 70;");
    ASSERT_EQ(res.rows.size(), 1u);
    EXPECT_EQ(std::get<int>(res.rows[0][0]), 3);
}

TEST_F(SelectTest, GroupByScore) {
    auto res = run("SELECT score, COUNT(*) FROM scores GROUP BY score ORDER BY score ASC;");
    ASSERT_EQ(res.rows.size(), 4u);
    EXPECT_EQ(std::get<int>(res.rows[0][0]), 60);
    EXPECT_EQ(std::get<int>(res.rows[0][1]), 1);
    EXPECT_EQ(std::get<int>(res.rows[2][0]), 80);
    EXPECT_EQ(std::get<int>(res.rows[2][1]), 2);
}

TEST_F(SelectTest, GroupByColumnAliasInResult) {
    auto res = run("SELECT score, COUNT(*) FROM scores GROUP BY score;");
    ASSERT_EQ(res.columns.size(), 2u);
    EXPECT_EQ(res.columns[0], "score");
    EXPECT_EQ(res.columns[1], "COUNT(*)");
}