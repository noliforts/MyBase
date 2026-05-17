#pragma once
#include "db_result.h"
#include <string>

class DbConnection {
public:
    virtual ~DbConnection() = default;
    virtual DbResult execute(const std::string& query) = 0;
    virtual void close() = 0;
};

class TcpDbConnection : public DbConnection {
    std::string host_;
    int port_;
public:
    TcpDbConnection(std::string h, int p) : host_(std::move(h)), port_(p) {}
    DbResult execute(const std::string& query) override;
    void close() override {}
};
