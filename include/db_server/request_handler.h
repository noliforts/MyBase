#pragma once
#include "protocol.h"
#include "db_core/session.h"

class SqlRequestHandler {
    Session session_;
public:
    Response handle(const Request& req);
};
