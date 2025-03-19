#pragma once

#include <library/cpp/logger/priority.h>

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/string/builder.h>

class TFsPath;
class TLogBackend;

namespace NOxygen {

    using TLogBackendPtr = TAtomicSharedPtr<TLogBackend>;

    TLogBackendPtr CreateAsyncFileLogBackend(const TFsPath& path);
    TLogBackendPtr WrapInThreadLogBackend(TLogBackendPtr backend);

    /**
     * A global logger (via {@link #Instance} static field). Allows to specify a thread local logger. By default
     * write log to stdout (like crawlrank).
     */
    class TOxygenLogger : private TNonCopyable {
    public:
        ~TOxygenLogger();

        void SetDefault(TLogBackendPtr backend);
        void SetDefaultToFile(const TFsPath& path);

        void SetThreadLocal(TLogBackendPtr backend);
        void SetThreadLocal(const TFsPath& path);
        void ResetThreadLocal();

        void Debug(const TString& message, const TStringBuf& fileName, size_t lineNumber, bool useThreadLocal = true) const;
        void Info(const TString& message, const TStringBuf& fileName, size_t lineNumber, bool useThreadLocal = true) const;
        void Warn(const TString& message, const TStringBuf& fileName, size_t lineNumber, bool useThreadLocal = true) const;
        void Error(const TString& message, const TStringBuf& fileName, size_t lineNumber, bool useThreadLocal = true) const;

        inline ELogPriority GetLogLevel() const {
            return LogLevel;
        }
        void SetLogLevel(ELogPriority level);

        bool IsLevelEnabled(ELogPriority level) const {
            return GetLogLevel() >= level;
        }
        bool IsDebugEnabled() const {
            return IsLevelEnabled(TLOG_DEBUG);
        }
        bool IsInfoEnabled() const {
            return IsLevelEnabled(TLOG_INFO);
        }
        bool IsWarnEnabled() const {
            return IsLevelEnabled(TLOG_WARNING);
        }
        bool IsErrorEnabled() const {
            return IsLevelEnabled(TLOG_ERR);
        }

        TLogBackend& GetBackend() {
            return *Backend;
        }

        static inline TOxygenLogger& GetInstance() {
            return Instance;
        }

    private:
        static TOxygenLogger Instance;
        TLogBackendPtr Backend;
        ELogPriority LogLevel;

        TOxygenLogger();

        void Write(ELogPriority level, const TStringBuf& levelName, const TString& message, const TStringBuf& fileName,
            size_t lineNumber, bool useThreadLocal) const;
    };

    class TOxygenLoggerThreadLocalHolder {
    public:
        TOxygenLoggerThreadLocalHolder(TLogBackendPtr backend);
        TOxygenLoggerThreadLocalHolder(const TFsPath& path);
        ~TOxygenLoggerThreadLocalHolder();
    };

    class TOxygenLoggerThreadLocalInheritHelper {
    private:
        TLogBackendPtr Backend;

    public:
        TOxygenLoggerThreadLocalInheritHelper();
        ~TOxygenLoggerThreadLocalInheritHelper();

        /** Should be called in the different thread. */
        void Inject() const;
    };

    namespace NLoggerImpl {
        inline TStringBuf StripFileName(TStringBuf path) {
            TStringBuf l, r;
            if (path.TryRSplit('/', l, r) || path.TryRSplit('\\', l, r)) {
                return r;
            } else {
                return path;
            }
        }
    }

    struct TDontUseThreadLocal {
    };

    class TOxygenLogEntry {
    public:
        using TLogFunc = void (TOxygenLogger::*)(const TString&, const TStringBuf&, size_t, bool) const;

    private:
        TStringBuilder Builder;
        bool Write;
        TStringBuf FileName;
        size_t LineNumber;
        TLogFunc LogFunc;
        bool UseThreadLocal = true;

    public:
        inline TOxygenLogEntry(ELogPriority logLevel, const TStringBuf& fileName, size_t lineNumber, TLogFunc logFunc)
            : Write(logLevel <= TOxygenLogger::GetInstance().GetLogLevel())
            , FileName(fileName)
            , LineNumber(lineNumber)
            , LogFunc(logFunc)
        {
        }

        ~TOxygenLogEntry() {
            if (!Write) {
                return;
            }

            try {
                (TOxygenLogger::GetInstance().*LogFunc)(Builder, FileName, LineNumber, UseThreadLocal);
            } catch (...) {
                // bad, but don't fail on log write error
            }
        }

        template <class T>
        inline TOxygenLogEntry& operator<<(const T& t) {
            if (Write) {
                Builder << t;
            }
            return *this;
        }

        inline TOxygenLogEntry& operator<<(const TDontUseThreadLocal&) {
            UseThreadLocal = false;
            return *this;
        }
    };

} // namespace NOxygen

#define L_LOGGER_IMPL(level, func) \
    NOxygen::TOxygenLogEntry( \
        level, \
        NOxygen::NLoggerImpl::StripFileName(__FILE__), \
        __LINE__, \
        &NOxygen::TOxygenLogger::func \
        )

#define L_DEBUG L_LOGGER_IMPL(TLOG_DEBUG, Debug)
#define L_INFO L_LOGGER_IMPL(TLOG_INFO, Info)
#define L_WARN L_LOGGER_IMPL(TLOG_WARNING, Warn)
#define L_ERROR L_LOGGER_IMPL(TLOG_ERR, Error)

#define OLOG_IMPL(level, message) \
    NOxygen::TOxygenLogger::GetInstance().level( \
        TStringBuilder() << message, \
        NOxygen::NLoggerImpl::StripFileName(TStringBuf(__FILE__)), \
        __LINE__ \
        )

#define OLOG_DEBUG(message) OLOG_IMPL(Debug, message)
#define OLOG_INFO(message) OLOG_IMPL(Info, message)
#define OLOG_WARN(message) OLOG_IMPL(Warn, message)
#define OLOG_ERROR(message) OLOG_IMPL(Error, message)
