#pragma once
#include "database.h"
#include <optional>
#include <string>

struct Session {
    std::string currentDb;
    bool inTransaction = false;
    std::optional<std::map<std::string, Database>> snapshot;
    std::string snapshotCurrentDb;
};