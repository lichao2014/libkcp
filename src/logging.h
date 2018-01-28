#ifndef _KCP_LOGGING_H_INCLUDED
#define _KCP_LOGGING_H_INCLUDED

#include <array>
#include <strstream>

namespace kcp {
enum class LogLevel : uint8_t {
    kDebug,
    kInfo,
    kWarning,
    kError,
    kNone
};

class LogSinkInterface {
protected:
    virtual ~LogSinkInterface() = default;
public:
    virtual void OnLog(const char *msg) = 0;
};

template<typename Buffer>
struct BufferStream : Buffer, std::ostrstream {
    template<typename ... Args>
    BufferStream()
        : Buffer(std::forward<Args>(args)...)
        , std::ostrstream(data(), size()) {}
};

class LogMessage {
public:
    static constexpr size_t kLogMaxSize = 1024;

    static void SetEnabled(bool on);
    static bool Loggable(LogLevel level);
    static void WithLevel(bool on);
    static void WithThread(bool on);
    static void WithTimestamp(bool on);
    static void WithFileAndLine(bool on);
    static void SetLogLevel(LogLevel min_log_level);
    static void SetSink(LogSinkInterface *sink);
    static void SetFileSink(const char *filename);

    LogMessage(LogLevel level, const char *file, size_t line);
    ~LogMessage();

    std::ostream& stream();
private:
    static bool enabled_;
    static bool level_print_;
    static bool thread_print_;
    static bool timestamp_print_;
    static bool file_and_line_print_;
    static LogLevel min_log_level_;
    static LogSinkInterface *sink_;

    LogLevel level_;
    BufferStream<std::array<char, kLogMaxSize>> stream_;
};

class LogMessageVoidify {
public:
    constexpr LogMessageVoidify() = default;
    constexpr void operator &(std::ostream&) {}
};
}

#define KCP_LOG(lvl)                                                \
    !kcp::LogMessage::Loggable(kcp::LogLevel::lvl)                  \
    ? (void)0 : kcp::LogMessageVoidify() &                          \
    kcp::LogMessage(kcp::LogLevel::lvl, __FILE__, __LINE__).stream()

#endif // !_KCP_LOGGING_H_INCLUDED
