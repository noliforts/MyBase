#include "db_server/request_handler.h"
#include "db_core/database_manager.h"
#include "db_core/lexer.h"
#include "db_core/parser.h"
#include <mutex>

Response SqlRequestHandler::handle(const Request& req) {
    try {
        Lexer lexer(req.sql);
        Parser<Lexer> parser(lexer);
        auto cmd = parser.parse();

        auto& mgr = DatabaseManager::instance();
        std::lock_guard<std::mutex> lock(mgr.mutex());
        Response res = cmd->execute(mgr, session_);
        mgr.saveAll(session_);
        return res;
    } catch (const std::exception& e) {
        return {false, false, {}, {}, {}, 0, e.what(), true};
    }
}