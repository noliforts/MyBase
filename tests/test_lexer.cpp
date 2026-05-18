#include <gtest/gtest.h>
#include "db_core/lexer.h"

TEST(LexerTest, KeywordsAndLiterals) {
    std::string src = "SELECT * FROM t WHERE id = -42 AND gpa = 3.14 OR is_active = TRUE OR name = 'Alex' OR val = NULL;";
    Lexer lexer(src);

    EXPECT_EQ(lexer.nextToken().type, TokenType::SELECT);
    EXPECT_EQ(lexer.nextToken().type, TokenType::STAR);
    EXPECT_EQ(lexer.nextToken().type, TokenType::FROM);

    Token t_id = lexer.nextToken();
    EXPECT_EQ(t_id.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t_id.lexeme, "t");

    EXPECT_EQ(lexer.nextToken().type, TokenType::WHERE);

    Token t_col = lexer.nextToken();
    EXPECT_EQ(t_col.type, TokenType::IDENTIFIER);
    EXPECT_EQ(t_col.lexeme, "id");

    EXPECT_EQ(lexer.nextToken().type, TokenType::EQUAL);

    Token t_int = lexer.nextToken();
    EXPECT_EQ(t_int.type, TokenType::INT_LIT);
    EXPECT_EQ(t_int.lexeme, "-42");

    EXPECT_EQ(lexer.nextToken().type, TokenType::AND);
    lexer.nextToken(); // gpa
    lexer.nextToken(); // =

    Token t_float = lexer.nextToken();
    EXPECT_EQ(t_float.type, TokenType::FLOAT_LIT);
    EXPECT_EQ(t_float.lexeme, "3.14");

    EXPECT_EQ(lexer.nextToken().type, TokenType::OR);
    lexer.nextToken(); // is_active
    lexer.nextToken(); // =

    Token t_bool = lexer.nextToken();
    EXPECT_EQ(t_bool.type, TokenType::BOOL_LIT);
    EXPECT_EQ(t_bool.lexeme, "TRUE");

    EXPECT_EQ(lexer.nextToken().type, TokenType::OR);
    lexer.nextToken(); // name
    lexer.nextToken(); // =

    Token t_str = lexer.nextToken();
    EXPECT_EQ(t_str.type, TokenType::STRING_LIT);
    EXPECT_EQ(t_str.lexeme, "Alex");

    EXPECT_EQ(lexer.nextToken().type, TokenType::OR);
    lexer.nextToken(); // val
    lexer.nextToken(); // =
    EXPECT_EQ(lexer.nextToken().type, TokenType::NULL_LIT);
    EXPECT_EQ(lexer.nextToken().type, TokenType::SEMICOLON);
}

TEST(LexerTest, LineAndColumnTracking) {
    std::string src = "SELECT\n  id\nFROM t;";
    Lexer lexer(src);

    Token t1 = lexer.nextToken();
    EXPECT_EQ(t1.line, 1);
    EXPECT_EQ(t1.column, 1);

    Token t2 = lexer.nextToken();
    EXPECT_EQ(t2.line, 2);
    EXPECT_EQ(t2.column, 3);

    Token t3 = lexer.nextToken();
    EXPECT_EQ(t3.line, 3);
    EXPECT_EQ(t3.column, 1);
}

TEST(LexerTest, TransactionKeywords) {
    std::string src = "BEGIN; COMMIT; ROLLBACK;";
    Lexer lexer(src);

    Token t1 = lexer.nextToken();
    EXPECT_EQ(t1.type, TokenType::BEGIN);
    EXPECT_EQ(t1.lexeme, "BEGIN");
    EXPECT_EQ(lexer.nextToken().type, TokenType::SEMICOLON);

    Token t2 = lexer.nextToken();
    EXPECT_EQ(t2.type, TokenType::COMMIT);
    EXPECT_EQ(t2.lexeme, "COMMIT");
    EXPECT_EQ(lexer.nextToken().type, TokenType::SEMICOLON);

    Token t3 = lexer.nextToken();
    EXPECT_EQ(t3.type, TokenType::ROLLBACK);
    EXPECT_EQ(t3.lexeme, "ROLLBACK");
    EXPECT_EQ(lexer.nextToken().type, TokenType::SEMICOLON);

    EXPECT_EQ(lexer.nextToken().type, TokenType::END_OF_FILE);
}

TEST(LexerTest, TransactionKeywordsCaseInsensitive) {
    std::string src = "begin; commit; rollback;";
    Lexer lexer(src);

    EXPECT_EQ(lexer.nextToken().type, TokenType::BEGIN);
    lexer.nextToken();
    EXPECT_EQ(lexer.nextToken().type, TokenType::COMMIT);
    lexer.nextToken();
    EXPECT_EQ(lexer.nextToken().type, TokenType::ROLLBACK);
}

TEST(LexerTest, ExceptionsThrowing) {
    std::string unclosed = "SELECT * FROM t WHERE name = 'Alex";
    Lexer lex1(unclosed);
    EXPECT_THROW({
        while(lex1.nextToken().type != TokenType::END_OF_FILE);
    }, std::runtime_error);

    std::string unexpected = "SELECT @ FROM t;";
    Lexer lex2(unexpected);
    lex2.nextToken();
    EXPECT_THROW(lex2.nextToken(), std::runtime_error);
}
