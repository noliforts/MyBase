#pragma once
#include "database.h"
#include <optional>
#include <string>

struct Session {
    std::string currentDb;
    bool inTransaction = false;
    // Полная копия всех баз данных на момент BEGIN. Откат работает простым
    // восстановлением этой копии, без журнала операций.
    std::optional<std::map<std::string, Database>> snapshot;
    std::string snapshotCurrentDb;
};