#include "db_server/request_handler.h"
#include "db_core/database_manager.h"
#include "db_core/lexer.h"
#include "db_core/parser.h"
#include <mutex>

Response SqlRequestHandler::handle(const Request& req) {
    try {
        auto& mgr = DatabaseManager::instance();
        std::lock_guard<std::mutex> lock(mgr.mutex());

        Lexer lexer(req.sql);
        Parser<Lexer> parser(lexer);

        Response res;
        bool executed = false;
        while (auto cmd = parser.parse()) {
            res = cmd->execute(mgr, session_);
            executed = true;
        }

        mgr.saveAll(session_);

        if (!executed)
            return {false, false, {}, {}, {}, 0, "Empty statement", true};
        return res;
    } catch (const std::exception& e) {
        return {false, false, {}, {}, {}, 0, e.what(), true};
    }
}