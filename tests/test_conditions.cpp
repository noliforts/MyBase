#include <gtest/gtest.h>
#include "db_core/condition.h"

TEST(ConditionTest, ComparisonOperatorsWithTypes) {
    Row row = {10, 3.14f, std::string("test"), nullptr};

    TableSchema schema;
    schema.columns = {
        {"a", DataType::INT},
        {"b", DataType::FLOAT},
        {"c", DataType::TEXT},
        {"d", DataType::INT}
    };

    ComparisonNode eq_node("a", "=", 10);
    EXPECT_TRUE(eq_node.evaluate(row, schema));

    ComparisonNode ne_node("b", "!=", 2.0f);
    EXPECT_TRUE(ne_node.evaluate(row, schema));

    ComparisonNode str_node("c", "=", std::string("test"));
    EXPECT_TRUE(str_node.evaluate(row, schema));

    ComparisonNode null_node("d", "=", nullptr);
    EXPECT_TRUE(null_node.evaluate(row, schema));
}

TEST(ConditionTest, LogicalTruthTable) {
    Row row = {1, 0};

    TableSchema schema;
    schema.columns = {
        {"t", DataType::INT},
        {"f", DataType::INT}
    };

    auto node_t = std::make_shared<ComparisonNode>("t", "=", 1);
    auto node_f = std::make_shared<ComparisonNode>("f", "=", 1);

    AndNode and_node(node_t, node_f);
    EXPECT_FALSE(and_node.evaluate(row, schema));

    OrNode or_node(node_t, node_f);
    EXPECT_TRUE(or_node.evaluate(row, schema));

    NotNode not_node(node_f);
    EXPECT_TRUE(not_node.evaluate(row, schema));
}

TEST(ConditionTest, DeepNesting) {
    Row row = {1, 1, 0};

    TableSchema schema;
    schema.columns = {
        {"a", DataType::INT},
        {"b", DataType::INT},
        {"c", DataType::INT}
    };

    auto node_a = std::make_shared<ComparisonNode>("a", "=", 1);
    auto node_b = std::make_shared<ComparisonNode>("b", "=", 1);
    auto node_c = std::make_shared<ComparisonNode>("c", "=", 1);

    auto left_and = std::make_shared<AndNode>(node_a, node_b);
    auto right_not = std::make_shared<NotNode>(node_c);

    OrNode root(left_and, right_not);
    EXPECT_TRUE(root.evaluate(row, schema));
}
