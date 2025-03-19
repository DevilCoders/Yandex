#pragma once

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>
#include <util/system/spinlock.h>
#include <util/stream/str.h>
#include <util/system/src_location.h>
#include <util/system/thread.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>

#include "searchlog.h"

class TErrorLog;

class TErrorLogElement : public TLogElement {
public:
    TErrorLogElement(TErrorLog* parent, ELogPriority priority, const TSourceLocation& location);

    void DoFlush() override {
        // Don't flush anything, ~TLogElement will write all the data
    }

    void FixHeaderLength() {
        HeaderLength = Filled();
    }

    ~TErrorLogElement() override;

private:
    TErrorLogElement(const TErrorLogElement& from) = delete;

protected:
    TErrorLog* ErrorLog = nullptr;
    TSourceLocation Location;
    size_t HeaderLength = 0;
};

class TErrorLog: private TSearchLog {
    static const size_t PREALLOCATED_MESSAGE_LENGTH = 8 * 1024;
public:
    using TMessageCallback = std::function<void(void)>;

    struct TRequestInfo {
        TString CollectionName;
        TString QueryString;
    };

    TErrorLog();

    void SetReplicateMessagesToErrorStream(bool value) {
        ReplicateMessagesToErrorStream = value;
    }

    THolder<TErrorLogElement> CreateLogEntry(ELogPriority priority, TSourceLocation location);
    void WriteFormattedLogEntry(ELogPriority priority, TSourceLocation location, bool printBackTrace, const char* format, ...) const;
    // Report problems with memory allocation. Memory allocator can be corrupted and no allocation can be made.
    // The function is intended not to use memory allocation, but there is no garantee that it won't require memory
    // allocation in the future (somebody can break something). Precautions should be taken when this method is used.
    // FIXME: Log backend not using memory allocations will make this method much safer.
    void ReportMemoryAllocationProblems();

    using TLog::OpenLog;
    using TLog::IsOpen;
    using TLog::ReopenLog;
    using TLog::ResetBackend;
    using TLog::ReleaseBackend;

    TErrorLog& operator=(const TLog& log) {
        TLog::operator=(log);
        return *this;
    }

    void RegisterRequestInfo(TRequestInfo&&);
    void PrintRequestInfo(IOutputStream& out, size_t maxSize = 0) const;
    void DeleteRequestInfo();
    void SetCallbackOnPriorityMessage(ELogPriority priority, TMessageCallback callback);
    friend class TErrorLogElement;

    void KeepLastEntry(ELogPriority priority, bool enable);
    void SetLastEntry(const TLogElement& logElem);
    TString LastEntry(ELogPriority priority);

public:
    bool ReplicateMessagesToErrorStream;
    THashMap<TThread::TId, TRequestInfo> RequestInfos;
    TMutex RequestInfosMutex;
    TRWMutex MessageCallbackMutex;
    TString PreallocatedMessage;
    THashMap<ELogPriority, TMessageCallback> MessageCallbacks;

protected:
    TLog* Log() noexcept {
        return this;
    }

private:
    static const size_t LogPrioritiesCount_ = (size_t)(LOG_MAX_PRIORITY) + 1;
    bool KeepLastEntry_[LogPrioritiesCount_];
    TSpinLock SL_;
    TString LastEntry_[LogPrioritiesCount_];
};

class TRequestInfoForErrorLog {
public:
    TRequestInfoForErrorLog(TString collectionName, TString queryString) {
        Singleton<TErrorLog>()->RegisterRequestInfo({std::move(collectionName), std::move(queryString)});
    }
    ~TRequestInfoForErrorLog() {
        Singleton<TErrorLog>()->DeleteRequestInfo();
    }
};

// Report verbose debug info
#define SEARCH_DEBUG \
    (*(Singleton<TErrorLog>()->CreateLogEntry(TLOG_DEBUG, __LOCATION__)).Get())

// Report some information.
#define SEARCH_INFO \
    (*(Singleton<TErrorLog>()->CreateLogEntry(TLOG_INFO, __LOCATION__)).Get())

// Report about a problem, which doesn't affect the search result.
// Such problem can cause only minor performance drawbacks.
#define SEARCH_WARNING \
    (*(Singleton<TErrorLog>()->CreateLogEntry(TLOG_WARNING, __LOCATION__)).Get())

// Report about a problem, which affects or can affect the search result.
// Although the search response generally won't be empty.
#define SEARCH_ERROR \
    (*(Singleton<TErrorLog>()->CreateLogEntry(TLOG_ERR, __LOCATION__)).Get())

// Report about a problem, which cause the empty search result.
#define SEARCH_CRITICAL \
    (*(Singleton<TErrorLog>()->CreateLogEntry(TLOG_CRIT, __LOCATION__)).Get())

// Report about a problem, which cause the search program to terminate execution.
#define SEARCH_EMERGENCY \
    (*(Singleton<TErrorLog>()->CreateLogEntry(TLOG_EMERG, __LOCATION__)).Get())

#define GET_ERRORLOG (*Singleton<TErrorLog>())

class TErrorLogWriter {
public:
    TErrorLogWriter(ELogPriority priority, TSourceLocation location)
        : Priority(priority)
        , Location(location)
    {
    }
    TErrorLogElement& operator()() {
        LastElement = Singleton<TErrorLog>()->CreateLogEntry(Priority, Location);
        return *(LastElement.Get());
    }

private:
    THolder<TErrorLogElement> LastElement;
    ELogPriority Priority;
    TSourceLocation Location;
};

#define SEARCH_CONFIG_WARNINGS(config) \
{\
    TErrorLogWriter writer(TLOG_WARNING, __LOCATION__);\
    (config).PrintErrors(writer);\
}

#define SEARCH_CONFIG_EMERGENCIES(config) \
{\
    TErrorLogWriter writer(TLOG_EMERG, __LOCATION__);\
    (config).PrintErrors(writer);\
}

#define ERROR_LOG_SUFFIX(...) ERROR_LOG_ARG_N(__VA_ARGS__, ANN, ANN, ANN, ANN, ANN, ANN, ANN, ANN, SILENT, SILENT)
#define ERROR_LOG_ARG_N(_expr, _0, _1,_2,_3, _4, _5, _6, _7, N, ...) N
#define ERROR_LOG_CONCAT(a, b) ERROR_LOG_CONCAT_IMPL(a, b)
#define ERROR_LOG_CONCAT_IMPL(a, b) a ## b

#define SEARCH_VERIFY_ANN(expr, ...) \
    do { \
        if (Y_UNLIKELY(!(expr))) { \
            Singleton<TErrorLog>()->WriteFormattedLogEntry(TLOG_EMERG, __LOCATION__, true, "Run-time assertion failed (" #expr "). " __VA_ARGS__); \
            abort(); \
        } \
    } while (false)

#define SEARCH_VERIFY_SILENT(expr, ...) \
    do { \
        if (Y_UNLIKELY(!(expr))) { \
            Singleton<TErrorLog>()->WriteFormattedLogEntry(TLOG_EMERG, __LOCATION__, true, "Run-time assertion failed (" #expr ")."); \
            abort(); \
        } \
    } while (false)

// The Actual format is SEARCH_VERIFY(expr) or SEARCH_VERIFY(expr, format, ...)
#define SEARCH_VERIFY(...) \
    ERROR_LOG_CONCAT(SEARCH_VERIFY_, ERROR_LOG_SUFFIX(__VA_ARGS__))(__VA_ARGS__)

// The Actual format is SEARCH_FAIL(format, ...)
#define SEARCH_FAIL(...) \
    do { \
        Singleton<TErrorLog>()->WriteFormattedLogEntry(TLOG_EMERG, __LOCATION__, true, "Run-time assertion failed. " __VA_ARGS__); \
        abort(); \
    } while (false)


/**
 * This instantiates log context only when someone wrote into it.
 * The problem is that source line information is lost
 **/
class TNonEmptyErrorLogger {
public:
    TNonEmptyErrorLogger(TStringBuf context, ELogPriority priority = TLOG_ERR)
        : Priority_(priority)
        , Context_(context)
    {
    }
    operator IOutputStream&() {
        return Errors_;
    }

    ~TNonEmptyErrorLogger() {
        if (Errors_.Str()) {
            *(Singleton<TErrorLog>()->CreateLogEntry(Priority_, __LOCATION__).Get()) <<
                '[' << Context_ << "]: " << Errors_.Str();
        }
    }
private:
    TStringStream Errors_;
    ELogPriority Priority_ = TLOG_ERR;
    TStringBuf Context_;
};
