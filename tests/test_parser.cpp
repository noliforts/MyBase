#include <gtest/gtest.h>
#include "db_core/parser.h"

class MockLexer {
    std::vector<Token> tokens_;
    size_t pos_ = 0;
public:
    explicit MockLexer(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}
    Token nextToken() {
        if (pos_ >= tokens_.size()) return {TokenType::END_OF_FILE, "", 0, 0};
        return tokens_[pos_++];
    }
    Token peekToken() {
        if (pos_ >= tokens_.size()) return {TokenType::END_OF_FILE, "", 0, 0};
        return tokens_[pos_];
    }
};

TEST(ParserTest, CommandTypesAndFields) {
    std::vector<Token> tokens = {
        {TokenType::USE, "USE", 1, 1},
        {TokenType::IDENTIFIER, "my_db", 1, 5},
        {TokenType::SEMICOLON, ";", 1, 10}
    };
    MockLexer mock(tokens);
    Parser<MockLexer> parser(mock);

    auto cmd = parser.parse();
    EXPECT_NE(dynamic_cast<UseDatabaseCommand*>(cmd.get()), nullptr);
}

TEST(ParserTest, NestedWhereConditions) {
    std::vector<Token> tokens = {
        {TokenType::SELECT, "SELECT", 1, 1}, {TokenType::STAR, "*", 1, 8}, {TokenType::FROM, "FROM", 1, 10},
        {TokenType::IDENTIFIER, "t", 1, 15}, {TokenType::WHERE, "WHERE", 1, 17},
        {TokenType::LPAREN, "(", 1, 23}, {TokenType::IDENTIFIER, "a", 1, 24}, {TokenType::EQUAL, "=", 1, 25}, {TokenType::INT_LIT, "1", 1, 26},
        {TokenType::AND, "AND", 1, 28}, {TokenType::IDENTIFIER, "b", 1, 32}, {TokenType::EQUAL, "=", 1, 33}, {TokenType::INT_LIT, "2", 1, 34},
        {TokenType::RPAREN, ")", 1, 35}, {TokenType::OR, "OR", 1, 37},
        {TokenType::LPAREN, "(", 1, 40}, {TokenType::NOT, "NOT", 1, 41}, {TokenType::IDENTIFIER, "c", 1, 45}, {TokenType::EQUAL, "=", 1, 46}, {TokenType::INT_LIT, "3", 1, 47},
        {TokenType::RPAREN, ")", 1, 48}, {TokenType::SEMICOLON, ";", 1, 49}
    };
    MockLexer mock(tokens);
    Parser<MockLexer> parser(mock);

    auto cmd = parser.parse();
    auto select_cmd = dynamic_cast<SelectCommand*>(cmd.get());
    EXPECT_NE(select_cmd, nullptr);
}

TEST(ParserTest, SyntaxErrors) {
    std::vector<Token> no_semicolon = {
        {TokenType::USE, "USE", 1, 1},
        {TokenType::IDENTIFIER, "db", 1, 5}
    };
    MockLexer mock1(no_semicolon);
    Parser<MockLexer> parser1(mock1);
    EXPECT_THROW(parser1.parse(), std::runtime_error);
}
