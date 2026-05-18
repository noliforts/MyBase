#pragma once
#include "types.h"
#include <string>
#include <vector>

enum class TokenType {
    CREATE, DROP, DATABASE, TABLE, USE, SELECT, INSERT, INTO, VALUES, UPDATE, SET, DELETE, FROM, WHERE,
    AND, OR, NOT, INT_TYPE, FLOAT_TYPE, BOOL_TYPE, TEXT_TYPE, VARCHAR_TYPE,
    BEGIN, COMMIT, ROLLBACK,
    IDENTIFIER, INT_LIT, FLOAT_LIT, STRING_LIT, BOOL_LIT, NULL_LIT,
    EQUAL, NOT_EQUAL, LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    COMMA, SEMICOLON, LPAREN, RPAREN, STAR, END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line = 1;
    int column = 1;
};

class Lexer {
    std::string input_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    char peek() const;
    char advance();
    void skipWhitespace();
public:
    explicit Lexer(std::string input) : input_(std::move(input)) {}
    Token nextToken();
    Token peekToken();
};
