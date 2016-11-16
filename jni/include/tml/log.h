#pragma once

#include <string>

namespace tml {

enum class LogLevel {
    TRACE, DEBUG, INFO, WARN, ERROR, FATAL
};

class LogPrinter {

public:
    virtual ~LogPrinter() { }
    virtual void printLogMessage(LogLevel level, const std::string& tag, const char* msg, va_list va) = 0;

};

class Log {

private:
    LogPrinter* printer;
    std::string tag;

    void print(LogLevel level, const char* msg, va_list va);

public:
    Log(LogPrinter* printer, std::string tag) : printer(printer), tag(tag) { }

    const std::string& getTag() const { return tag; }

    void trace(const char* msg, ...);
    void debug(const char* msg, ...);
    void info(const char* msg, ...);
    void warn(const char* msg, ...);
    void error(const char* msg, ...);
    void fatal(const char* msg, ...);

};

}