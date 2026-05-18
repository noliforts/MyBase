#pragma once
#include "lexer.h"
#include "commands.h"
#include <stdexcept>
#include <memory>
#include <vector>
#include <string>

template<typename LexerT>
class Parser {
    LexerT& lexer_;

    void expect(TokenType type);
    bool match(TokenType type);
    DataType parseType();
    Value parseLiteral();

    std::shared_ptr<ConditionNode> parseExpression();
    std::shared_ptr<ConditionNode> parseAndExpression();
    std::shared_ptr<ConditionNode> parsePrimaryCondition();
    std::shared_ptr<ConditionNode> parseOptionalWhere();
    SelectExpr parseSelectExpr();

public:
    explicit Parser(LexerT& lexer);
    std::unique_ptr<Command> parse();
};

template<typename LexerT>
Parser<LexerT>::Parser(LexerT& lexer) : lexer_(lexer) {}

template<typename LexerT>
void Parser<LexerT>::expect(TokenType type) {
    if (lexer_.nextToken().type != type) throw std::runtime_error("Syntax error: unexpected token");
}

template<typename LexerT>
bool Parser<LexerT>::match(TokenType type) {
    if (lexer_.peekToken().type == type) { lexer_.nextToken(); return true; }
    return false;
}

template<typename LexerT>
DataType Parser<LexerT>::parseType() {
    if (match(TokenType::INT_TYPE)) return DataType::INT;
    if (match(TokenType::FLOAT_TYPE)) return DataType::FLOAT;
    if (match(TokenType::BOOL_TYPE)) return DataType::BOOL;
    if (match(TokenType::TEXT_TYPE)) return DataType::TEXT;
    if (match(TokenType::VARCHAR_TYPE)) {
        expect(TokenType::LPAREN);
        lexer_.nextToken();
        expect(TokenType::RPAREN);
        return DataType::TEXT;
    }
    throw std::runtime_error("Unknown data type");
}

template<typename LexerT>
Value Parser<LexerT>::parseLiteral() {
    Token t = lexer_.nextToken();
    if (t.type == TokenType::INT_LIT) return std::stoi(t.lexeme);
    if (t.type == TokenType::FLOAT_LIT) return std::stof(t.lexeme);
    if (t.type == TokenType::BOOL_LIT) return t.lexeme == "TRUE" || t.lexeme == "true";
    if (t.type == TokenType::STRING_LIT) return t.lexeme;
    if (t.type == TokenType::NULL_LIT) return nullptr;
    throw std::runtime_error("Expected literal value");
}

template<typename LexerT>
std::shared_ptr<ConditionNode> Parser<LexerT>::parseExpression() {
    auto left = parseAndExpression();
    while (match(TokenType::OR)) {
        auto right = parseAndExpression();
        left = std::make_shared<OrNode>(std::move(left), std::move(right));
    }
    return left;
}

template<typename LexerT>
std::shared_ptr<ConditionNode> Parser<LexerT>::parseAndExpression() {
    auto left = parsePrimaryCondition();
    while (match(TokenType::AND)) {
        auto right = parsePrimaryCondition();
        left = std::make_shared<AndNode>(std::move(left), std::move(right));
    }
    return left;
}

template<typename LexerT>
std::shared_ptr<ConditionNode> Parser<LexerT>::parsePrimaryCondition() {
    if (match(TokenType::NOT)) {
        return std::make_shared<NotNode>(parsePrimaryCondition());
    }
    if (match(TokenType::LPAREN)) {
        auto node = parseExpression();
        expect(TokenType::RPAREN);
        return node;
    }

    std::string col = lexer_.nextToken().lexeme;
    Token opToken = lexer_.nextToken();
    std::string op = opToken.lexeme;
    Value val = parseLiteral();
    return std::make_shared<ComparisonNode>(col, op, val);
}

template<typename LexerT>
std::shared_ptr<ConditionNode> Parser<LexerT>::parseOptionalWhere() {
    if (match(TokenType::WHERE)) return parseExpression();
    return nullptr;
}

template<typename LexerT>
SelectExpr Parser<LexerT>::parseSelectExpr() {
    static const std::pair<TokenType, AggFunc> aggTokens[] = {
        {TokenType::COUNT, AggFunc::COUNT}, {TokenType::SUM, AggFunc::SUM},
        {TokenType::MIN,   AggFunc::MIN},   {TokenType::MAX, AggFunc::MAX},
        {TokenType::AVG,   AggFunc::AVG},
    };
    for (auto& [tok, func] : aggTokens) {
        if (lexer_.peekToken().type == tok) {
            Token funcTok = lexer_.nextToken();
            std::string upper;
            for (char c : funcTok.lexeme) upper += std::toupper(c);
            expect(TokenType::LPAREN);
            Token colTok = lexer_.nextToken();
            std::string col = (colTok.type == TokenType::STAR) ? "*" : colTok.lexeme;
            expect(TokenType::RPAREN);
            return {true, col, func, upper + "(" + col + ")"};
        }
    }
    std::string col = lexer_.nextToken().lexeme;
    return {false, col, AggFunc::COUNT, col};
}

template<typename LexerT>
std::unique_ptr<Command> Parser<LexerT>::parse() {
    if (lexer_.peekToken().type == TokenType::END_OF_FILE)
        return nullptr;

    Token t = lexer_.peekToken();

    if (match(TokenType::CREATE)) {
        if (match(TokenType::DATABASE)) {
            std::string name = lexer_.nextToken().lexeme;
            expect(TokenType::SEMICOLON);
            return std::make_unique<CreateDatabaseCommand>(name);
        }
        if (match(TokenType::TABLE)) {
            std::string name = lexer_.nextToken().lexeme;
            expect(TokenType::LPAREN);
            std::vector<ColumnSchema> cols;
            do {
                std::string colName = lexer_.nextToken().lexeme;
                DataType type = parseType();
                cols.push_back({colName, type});
            } while (match(TokenType::COMMA));
            expect(TokenType::RPAREN);
            expect(TokenType::SEMICOLON);
            return std::make_unique<CreateTableCommand>(name, cols);
        }
    }
    else if (match(TokenType::DROP)) {
        if (match(TokenType::DATABASE)) {
            std::string name = lexer_.nextToken().lexeme;
            expect(TokenType::SEMICOLON);
            return std::make_unique<DropDatabaseCommand>(name);
        }
        if (match(TokenType::TABLE)) {
            std::string name = lexer_.nextToken().lexeme;
            expect(TokenType::SEMICOLON);
            return std::make_unique<DropTableCommand>(name);
        }
        throw std::runtime_error("Syntax error: expected DATABASE or TABLE after DROP");
    }
    else if (match(TokenType::USE)) {
        std::string name = lexer_.nextToken().lexeme;
        expect(TokenType::SEMICOLON);
        return std::make_unique<UseDatabaseCommand>(name);
    }
    else if (match(TokenType::INSERT)) {
        expect(TokenType::INTO);
        std::string table = lexer_.nextToken().lexeme;

        std::vector<std::string> targetColumns;
        if (match(TokenType::LPAREN)) {
            do {
                targetColumns.push_back(lexer_.nextToken().lexeme);
            } while (match(TokenType::COMMA));
            expect(TokenType::RPAREN);
        }

        expect(TokenType::VALUES);

        std::vector<std::vector<Value>> allRowsVals;
        do {
            expect(TokenType::LPAREN);
            std::vector<Value> rowVals;
            do {
                rowVals.push_back(parseLiteral());
            } while (match(TokenType::COMMA));
            expect(TokenType::RPAREN);
            allRowsVals.push_back(rowVals);
        } while (match(TokenType::COMMA));

        expect(TokenType::SEMICOLON);
        return std::make_unique<InsertCommand>(table, targetColumns, allRowsVals);
    }
    else if (match(TokenType::SELECT)) {
        std::vector<SelectExpr> exprs;
        if (match(TokenType::STAR))
            exprs.push_back({false, "*", AggFunc::COUNT, "*"});
        else {
            do { exprs.push_back(parseSelectExpr()); } while (match(TokenType::COMMA));
        }
        expect(TokenType::FROM);
        std::string table = lexer_.nextToken().lexeme;
        auto cond = parseOptionalWhere();

        std::optional<std::string> groupBy;
        if (match(TokenType::GROUP)) {
            expect(TokenType::BY);
            groupBy = lexer_.nextToken().lexeme;
        }

        std::optional<OrderBy> orderBy;
        if (match(TokenType::ORDER)) {
            expect(TokenType::BY);
            std::string col = lexer_.nextToken().lexeme;
            bool asc = true;
            if (match(TokenType::DESC)) asc = false;
            else match(TokenType::ASC);
            orderBy = OrderBy{col, asc};
        }

        std::optional<size_t> limit;
        if (match(TokenType::LIMIT))
            limit = static_cast<size_t>(std::stoul(lexer_.nextToken().lexeme));

        std::optional<size_t> offset;
        if (match(TokenType::OFFSET))
            offset = static_cast<size_t>(std::stoul(lexer_.nextToken().lexeme));

        expect(TokenType::SEMICOLON);
        return std::make_unique<SelectCommand>(table, exprs, cond, groupBy, orderBy, limit, offset);
    }
    else if (match(TokenType::UPDATE)) {
        std::string table = lexer_.nextToken().lexeme;
        expect(TokenType::SET);

        std::vector<std::pair<std::string, Value>> assignments;
        do {
            std::string col = lexer_.nextToken().lexeme;
            expect(TokenType::EQUAL);
            Value val = parseLiteral();
            assignments.push_back({col, val});
        } while (match(TokenType::COMMA));

        auto cond = parseOptionalWhere();
        expect(TokenType::SEMICOLON);
        return std::make_unique<UpdateCommand>(table, assignments, cond);
    }
    else if (match(TokenType::DELETE)) {
        expect(TokenType::FROM);
        std::string table = lexer_.nextToken().lexeme;
        auto cond = parseOptionalWhere();
        expect(TokenType::SEMICOLON);
        return std::make_unique<DeleteCommand>(table, cond);
    }
    else if (match(TokenType::BEGIN)) {
        expect(TokenType::SEMICOLON);
        return std::make_unique<BeginCommand>();
    }
    else if (match(TokenType::COMMIT)) {
        expect(TokenType::SEMICOLON);
        return std::make_unique<CommitCommand>();
    }
    else if (match(TokenType::ROLLBACK)) {
        expect(TokenType::SEMICOLON);
        return std::make_unique<RollbackCommand>();
    }
    throw std::runtime_error("Unsupported SQL statement");
}
