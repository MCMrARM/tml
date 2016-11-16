#pragma once

#include <string>

namespace tml {

enum class LogLevel {
    TRACE, DEBUG, INFO, WARN, ERROR, FATAL
};

class Log {

private:
    std::string tag;

public:
    Log(std::string tag) : tag(tag) { }

    const std::string& getTag() const { return tag; }

    void print(LogLevel level, const char* msg, ...);

    void trace(const char* msg, ...);
    void debug(const char* msg, ...);
    void info(const char* msg, ...);
    void warn(const char* msg, ...);
    void error(const char* msg, ...);
    void fatal(const char* msg, ...);

};

}