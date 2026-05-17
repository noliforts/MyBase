#pragma once
#include <string>
#include "db_core/types.h"
#include "protocol.h"

class SqlRequestHandler {
public:
    Response handle(const Request& req);
};
