#pragma once
#include "db_core/types.h"
#include <vector>
#include <cstdint>

struct Request { std::string sql; };
using Response = QueryResult;

class Protocol {
public:
    virtual ~Protocol() = default;
    virtual Request parseRequest(const std::vector<uint8_t>& raw) = 0;
    virtual std::vector<uint8_t> serializeResponse(const Response& resp) = 0;
};

class BinaryProtocol : public Protocol {
public:
    Request parseRequest(const std::vector<uint8_t>& raw) override;
    std::vector<uint8_t> serializeResponse(const Response& resp) override;
};
