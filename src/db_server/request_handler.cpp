#include "db_server/request_handler.h"
#include "db_core/database_manager.h"
#include "db_core/lexer.h"
#include "db_core/parser.h"

Response SqlRequestHandler::handle(const Request& req) {
    try {
        Lexer lexer(req.sql);
        Parser<Lexer> parser(lexer);
        auto cmd = parser.parse();
        return cmd->execute(DatabaseManager::instance());
    } catch (const std::exception& e) {
        // Возвращаем объект QueryResult в случае падения парсера
        return {false, false, {}, {}, {}, 0, e.what(), true};
    }
}
