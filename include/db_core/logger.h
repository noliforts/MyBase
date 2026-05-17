#pragma once
#include <iostream>
#include <fstream>

class Logger {
public:
    virtual ~Logger() = default;
    virtual void log(const std::string& msg) = 0;
};

class ConsoleLogger : public Logger {
public:
    void log(const std::string& msg) override { std::cout << "[LOG] " << msg << std::endl; }
};

class FileLogger : public Logger {
    std::ofstream out;
public:
    explicit FileLogger(const std::string& path) : out(path, std::ios::app) {}
    void log(const std::string& msg) override { if (out) out << "[LOG] " << msg << std::endl; }
};
