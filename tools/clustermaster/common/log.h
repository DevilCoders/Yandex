#pragma once

#include <library/cpp/charset/ci_string.h>

#include <util/datetime/base.h>
#include <util/generic/noncopyable.h>
#include <util/stream/null.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/system/file.h>

class TStdErrOutput: public IOutputStream {
public:
    TStdErrOutput() noexcept
#ifdef _MSC_VER
        : Handle(GetStdHandle(STD_ERROR_HANDLE))
#else
        : Handle(STDERR_FILENO)
#endif
    {
    }

    ~TStdErrOutput() override {
        Handle.Release();
    }

private:
    void DoWrite(const void* buf, size_t len) noexcept override {
        Handle.Write(buf, len);
    }

    void DoFlush() noexcept override {
        Handle.Flush();
    }

    TFileHandle Handle;
};

extern TStdErrOutput stderrOutput;

enum ELogLevel {
    All /* "all" */,
    Trace /* "trace" */,
    Debug /* "debug" */,
    Info /* "info" */,
    Error /* "error" */,
    None /* "none" */,
};

namespace LogSubsystem {
class All {};
class net {};
class http {};
};

struct TLogOutput: TNonCopyable {

    TLogOutput() noexcept
        : s()
        , out(s)
    {
        struct tm tm;
        out << Strftime("%Y-%m-%d %H:%M:%S ", TInstant::Now().LocalTime(&tm));
    }

    ~TLogOutput() {
        out << '\n';
        try {
            stderrOutput << s;
        } catch (...) {}
    }

    operator IOutputStream&() noexcept {
        return out;
    }

    TString s;
    TStringOutput out;
};

template <class T>
struct TLogLevel
{
    static ELogLevel& Level() noexcept {
        static ELogLevel level = None;
        return level;
    }
};

typedef TLogLevel<LogSubsystem::All> LogLevel;

template <>
struct TLogLevel<LogSubsystem::All>
{
    static ELogLevel& Level() noexcept {
        static ELogLevel level = Error;
        return level;
    }
};

ELogLevel GetLogLevelFromEnvOrDefault(ELogLevel defaultLevel);

#define LOGGG(LEVEL,LEVELNAME,HINT,MESSAGE) \
    do { \
        if (HINT(TLogLevel<LogSubsystem::All>::Level() <= LEVEL)) { \
            TLogOutput() << LEVELNAME ": " << MESSAGE; \
        } \
    } while(0)

#define LOGGG1(LEVEL,LEVELNAME,SUBSYSTEM,HINT,MESSAGE) \
    do { \
        if (HINT(TLogLevel<LogSubsystem::All>::Level() <= LEVEL || \
                TLogLevel<LogSubsystem::SUBSYSTEM>::Level() <= LEVEL)) { \
            TLogOutput() << LEVELNAME ": " << #SUBSYSTEM ": " << MESSAGE; \
        } \
    } while(0)

#define TRACELOG(MESSAGE) LOGGG(Trace,"TRACE",Y_UNLIKELY,MESSAGE)
#define TRACELOG1(SUBSYSTEM,MESSAGE) LOGGG1(Trace,"TRACE",SUBSYSTEM,Y_UNLIKELY,MESSAGE)

#define DEBUGLOG(MESSAGE) LOGGG(Debug,"DEBUG",Y_UNLIKELY,MESSAGE)
#define DEBUGLOG1(SUBSYSTEM,MESSAGE) LOGGG1(Debug,"DEBUG",SUBSYSTEM,Y_UNLIKELY,MESSAGE)

#define LOG(MESSAGE) LOGGG(Info,"INFO",Y_UNLIKELY,MESSAGE)
#define LOG1(SUBSYSTEM,MESSAGE) LOGGG1(Info,"INFO",SUBSYSTEM,Y_UNLIKELY,MESSAGE)

#define ERRORLOG(MESSAGE) LOGGG(Error,"ERROR",,MESSAGE)
#define ERRORLOG1(SUBSYSTEM,MESSAGE) LOGGG1(Error,"ERROR",SUBSYSTEM,,MESSAGE)
