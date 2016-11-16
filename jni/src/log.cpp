#include <tml/log.h>

using namespace tml;

#define TMLLog(func, level) \
void Log::func(const char* msg, ...) { \
    va_list va; \
    va_start(va, msg); \
    print(level, msg, va); \
    va_end(va); \
}

TMLLog(trace, LogLevel::TRACE)
TMLLog(debug, LogLevel::DEBUG)
TMLLog(info, LogLevel::INFO)
TMLLog(warn, LogLevel::WARN)
TMLLog(error, LogLevel::ERROR)
TMLLog(fatal, LogLevel::FATAL)

void Log::print(LogLevel level, const char* msg, va_list va) {
    printer->printLogMessage(level, tag, msg, va);
}