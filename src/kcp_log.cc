#include "kcp_log.h"
#include "common_types.h"

#include <iostream>
#include <fstream>
#include <thread>

using namespace kcp;

namespace {
class FileLogSink : public LogSinkInterface {
public:
    FileLogSink(const char *filename) : out_(filename) {}

    void OnLog(const char *msg) override {
        out_ << msg;
    }
private:
    std::ofstream out_;
};

const char *ToString(LogLevel level) {
    switch (level) {
    case LogLevel::kDebug:
        return "DEBUG";
        break;
    case LogLevel::kInfo:
        return "INFO";
    case LogLevel::kWarning:
        return "WARNING";
        break;
    case LogLevel::kError:
        return "ERROR";
        break;
    default:
        break;
    }

    return "NONE";
}
}

// static
bool LogMessage::enabled_ = false;

// static
bool LogMessage::level_print_ = true;

// static
bool LogMessage::thread_print_ = true;

// static
bool LogMessage::timestamp_print_ = true;

// static
bool LogMessage::file_and_line_print_ = false;

// static
LogLevel LogMessage::min_log_level_ = LogLevel::kDebug;

// static
LogSinkInterface *LogMessage::sink_ = nullptr;

LogMessage::LogMessage(LogLevel level, const char *file, size_t line)
    : stream_(buf_.data(), buf_.size())
    , level_(level) {
    if (!Loggable(level)) {
        return;
    }

    if (level_print_) {
        stream_ << ToString(level) << '|';
    }

    if (thread_print_) {
        stream_ << std::this_thread::get_id() << '|';
    }

    if (timestamp_print_) {
        stream_ << Now32() << '|';
    }

    if (file_and_line_print_) {
        stream_ << file << ',' << line << '|';
    }
}

LogMessage::~LogMessage() {
    if (sink_) {
        stream_ << std::ends;
        sink_->OnLog(buf_.data());
    }
}

// static
void LogMessage::SetEnabled(bool on) {
    enabled_ = on;
}

// static
bool LogMessage::Loggable(LogLevel level) {
    return enabled_ && level >= min_log_level_;
}

// static
void LogMessage::WithLevel(bool on) {
    level_print_ = on;
}

// static
void LogMessage::WithThread(bool on) {
    thread_print_ = on;
}

// static
void LogMessage::WithTimestamp(bool on) {
    timestamp_print_ = on;
}

// static
void LogMessage::WithFileAndLine(bool on) {
    file_and_line_print_ = on;
}

// static
void LogMessage::SetLogLevel(LogLevel min_log_level) {
    min_log_level_ = min_log_level;
}

// static
void LogMessage::SetSink(LogSinkInterface *sink) {
    sink_ = sink;
}

// static
void LogMessage::SetFileSink(const char *filename) {
    SetSink(new FileLogSink(filename));
}

std::ostream& LogMessage::stream() {
    if (!sink_) {
        return std::clog;
    }

    return stream_;
}