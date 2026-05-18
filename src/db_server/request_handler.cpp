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
        // Цикл нужен, чтобы USE db; и следующий запрос выполнялись в одной сессии.
        // Каждое TCP-соединение создаёт новый SqlRequestHandler со своей Session,
        // поэтому без мультистейтмента USE не сохранял бы состояние между запросами.
        while (auto cmd = parser.parse()) {
            res = cmd->execute(mgr, session_);
            executed = true;
        }

        // Если батч заканчивается без COMMIT (например, preview-SELECT клиента),
        // откатываем транзакцию, чтобы DatabaseManager остался в согласованном состоянии.
        if (session_.inTransaction) {
            RollbackCommand().execute(mgr, session_);
        }

        mgr.saveAll(session_);

        if (!executed)
            return {false, false, {}, {}, {}, 0, "Empty statement", true};
        return res;
    } catch (const std::exception& e) {
        return {false, false, {}, {}, {}, 0, e.what(), true};
    }
}