#include "db_core/lexer.h"
#include <cctype>
#include <unordered_map>

char Lexer::peek() const { return pos_ < input_.size() ? input_[pos_] : '\0'; }
char Lexer::advance() { char c = peek(); pos_++; col_++; if (c == '\n') { line_++; col_ = 1; } return c; }

void Lexer::skipWhitespace() {
    while (std::isspace(peek())) advance();
}

Token Lexer::nextToken() {
    skipWhitespace();
    if (pos_ >= input_.size()) return {TokenType::END_OF_FILE, "", line_, col_};

    int startCol = col_;
    char c = peek();

    if (std::isalpha(c) || c == '_') {
        std::string ident;
        while (std::isalnum(peek()) || peek() == '_') ident += advance();

        std::string upper;
        for (char ch : ident) upper += std::toupper(ch);

        static const std::unordered_map<std::string, TokenType> keywords = {
            {"CREATE", TokenType::CREATE}, {"DROP", TokenType::DROP}, {"DATABASE", TokenType::DATABASE},
            {"TABLE", TokenType::TABLE}, {"SELECT", TokenType::SELECT}, {"INSERT", TokenType::INSERT},
            {"USE", TokenType::USE},
            {"INTO", TokenType::INTO}, {"VALUES", TokenType::VALUES}, {"UPDATE", TokenType::UPDATE},
            {"SET", TokenType::SET}, {"DELETE", TokenType::DELETE}, {"FROM", TokenType::FROM},
            {"WHERE", TokenType::WHERE}, {"AND", TokenType::AND}, {"OR", TokenType::OR},
            {"NOT", TokenType::NOT}, {"INT", TokenType::INT_TYPE}, {"FLOAT", TokenType::FLOAT_TYPE},
            {"BOOL", TokenType::BOOL_TYPE}, {"TEXT", TokenType::TEXT_TYPE}, {"VARCHAR", TokenType::VARCHAR_TYPE},
            {"TRUE", TokenType::BOOL_LIT}, {"FALSE", TokenType::BOOL_LIT}, {"NULL", TokenType::NULL_LIT},
            {"BEGIN", TokenType::BEGIN}, {"COMMIT", TokenType::COMMIT}, {"ROLLBACK", TokenType::ROLLBACK},
            {"ORDER", TokenType::ORDER}, {"BY", TokenType::BY},
            {"ASC", TokenType::ASC}, {"DESC", TokenType::DESC},
            {"LIMIT", TokenType::LIMIT}, {"OFFSET", TokenType::OFFSET},
            {"GROUP", TokenType::GROUP},
            {"COUNT", TokenType::COUNT}, {"SUM", TokenType::SUM},
            {"MIN", TokenType::MIN}, {"MAX", TokenType::MAX}, {"AVG", TokenType::AVG}
        };

        if (keywords.count(upper)) return {keywords.at(upper), ident, line_, startCol};
        return {TokenType::IDENTIFIER, ident, line_, startCol};
    }

    if (std::isdigit(c) || (c == '-' && std::isdigit(input_[pos_+1]))) {
        std::string num;
        if (c == '-') num += advance();
        bool isFloat = false;
        while (std::isdigit(peek()) || peek() == '.') {
            if (peek() == '.') isFloat = true;
            num += advance();
        }
        return {isFloat ? TokenType::FLOAT_LIT : TokenType::INT_LIT, num, line_, startCol};
    }

    if (c == '\'') {
        advance();
        std::string str;
        while (peek() != '\'' && peek() != '\0') str += advance();
        if (peek() == '\0')
            throw std::runtime_error("Unterminated string literal at line " + std::to_string(line_));
        advance();
        return {TokenType::STRING_LIT, str, line_, startCol};
    }

    std::string op(1, advance());
    if ((op == "!" || op == "<" || op == ">") && peek() == '=') op += advance();

    TokenType type = TokenType::UNKNOWN;
    if (op == "=") type = TokenType::EQUAL;
    else if (op == "!=") type = TokenType::NOT_EQUAL;
    else if (op == "<") type = TokenType::LESS;
    else if (op == ">") type = TokenType::GREATER;
    else if (op == "<=") type = TokenType::LESS_EQUAL;
    else if (op == ">=") type = TokenType::GREATER_EQUAL;
    else if (op == ",") type = TokenType::COMMA;
    else if (op == ";") type = TokenType::SEMICOLON;
    else if (op == "(") type = TokenType::LPAREN;
    else if (op == ")") type = TokenType::RPAREN;
    else if (op == "*") type = TokenType::STAR;
    else throw std::runtime_error("Unexpected character '" + op + "' at line " + std::to_string(line_));

    return {type, op, line_, startCol};
}

Token Lexer::peekToken() {
    size_t oldPos = pos_;
    int oldLine = line_, oldCol = col_;
    Token t = nextToken();
    pos_ = oldPos; line_ = oldLine; col_ = oldCol;
    return t;
}
