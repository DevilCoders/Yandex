#include "log.h"

#include "flushing_stream_backend.h"

#include <library/cpp/logger/file.h>
#include <library/cpp/logger/record.h>
#include <library/cpp/logger/thread.h>

#include <util/datetime/base.h>
#include <util/folder/path.h>
#include <util/generic/buffer.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/tls.h>

using namespace NOxygen;

namespace {

    Y_STATIC_THREAD(TLogBackendPtr) ThreadLocalBackend;

    TLogBackendPtr& GetTlBackend() {
        if (!ThreadLocalBackend.GetPtr()) {
            ThreadLocalBackend = TLogBackendPtr();
        }
        return ThreadLocalBackend.Get();
    }

    template <typename TResultInput>
    class TLogBackendHolder : private TLogBackendPtr, public TResultInput {
    public:
        inline TLogBackendHolder(TLogBackendPtr input)
            : TSharedPtr(input)
            , TResultInput(input.Get())
        {
        }

        ~TLogBackendHolder() override {
        }
    };

    void FormatMessage(const TStringBuf& level, const TString& message,
        const TStringBuf& fileName, size_t lineNumber, TBuffer& buf)
    {
        static const size_t DATE_TIME_LENGTH = 19;
        static const size_t MILLIS_LENGTH = 3;
        static const size_t LINE_NUMBER_MAX_LENGTH = 10;
        static constexpr TStringBuf DATE_LEVEL_DELIMITER = "  ";
        static constexpr TStringBuf MESSAGE_FILE_DELIMITER = " - ";

        const TInstant now = TInstant::Now();
        struct tm tm;
        size_t len;

        TString lineNumberStr = IntToString<10>(lineNumber);
        buf.Reserve(DATE_TIME_LENGTH + 1 + MILLIS_LENGTH + 1 + level.size() + DATE_LEVEL_DELIMITER.size() + message.size() +
            MESSAGE_FILE_DELIMITER.size() + fileName.size() + 1 + LINE_NUMBER_MAX_LENGTH + 1);

        len = strftime(buf.Pos(), DATE_TIME_LENGTH + 1, "%Y-%m-%d %H:%M:%S", now.LocalTime(&tm));
        Y_VERIFY_DEBUG(len == DATE_TIME_LENGTH,
            "strftime expected %" PRISZT ", but found %" PRISZT, DATE_TIME_LENGTH, len);
        buf.Advance(DATE_TIME_LENGTH);

        buf.Append(',');

        len = snprintf(buf.Pos(), MILLIS_LENGTH + 1, "%03" PRIu32, now.MilliSecondsOfSecond());
        Y_VERIFY_DEBUG(len == MILLIS_LENGTH,
            "snprintf millis expected %" PRISZT ", but found %" PRISZT, MILLIS_LENGTH, len);
        buf.Advance(MILLIS_LENGTH);

        buf.Append(' ');
        buf.Append(level.data(), level.size());
        buf.Append(DATE_LEVEL_DELIMITER.data(), DATE_LEVEL_DELIMITER.size());
        buf.Append(message.data(), message.size());
        buf.Append(MESSAGE_FILE_DELIMITER.data(), MESSAGE_FILE_DELIMITER.size());
        buf.Append(fileName.data(), fileName.size());
        buf.Append(':');

        len = IntToString<10>(lineNumber, buf.Pos(), LINE_NUMBER_MAX_LENGTH + 1);
        Y_VERIFY_DEBUG(len > 0 && len <= LINE_NUMBER_MAX_LENGTH,
            "IntToString expected %" PRISZT ", but found %" PRISZT, LINE_NUMBER_MAX_LENGTH, len);
        buf.Advance(len);

        buf.Append('\n');
    }

} // namespace anonymous


TLogBackendPtr NOxygen::CreateAsyncFileLogBackend(const TFsPath& path) {
    TLogBackendPtr backend = new TFileLogBackend(path.GetPath());
    return WrapInThreadLogBackend(backend);
}

TLogBackendPtr NOxygen::WrapInThreadLogBackend(TLogBackendPtr backend) {
    return new TLogBackendHolder<TThreadedLogBackend>(backend);
}

NOxygen::TOxygenLogger NOxygen::TOxygenLogger::Instance;

NOxygen::TOxygenLogger::TOxygenLogger()
    : LogLevel(TLOG_INFO)
{
    TLogBackendPtr cerrBackend = new TFlushingStreamLogBackend(&Cerr);
    Backend = WrapInThreadLogBackend(cerrBackend);
}

NOxygen::TOxygenLogger::~TOxygenLogger() {
}

void NOxygen::TOxygenLogger::SetDefault(TLogBackendPtr backend) {
    Y_ENSURE(!!backend, "Logging back-end is null");
    Backend = backend;
}

void NOxygen::TOxygenLogger::SetDefaultToFile(const TFsPath& path) {
    SetDefault(CreateAsyncFileLogBackend(path));
}

void NOxygen::TOxygenLogger::SetThreadLocal(TLogBackendPtr backend) {
    Y_ENSURE(!!backend, "Logging back-end is null");
    GetTlBackend().Swap(backend);
}

void NOxygen::TOxygenLogger::SetThreadLocal(const TFsPath& path) {
    SetThreadLocal(CreateAsyncFileLogBackend(path));
}

void NOxygen::TOxygenLogger::ResetThreadLocal() {
    GetTlBackend().Drop();
}

void NOxygen::TOxygenLogger::Debug(const TString& message, const TStringBuf& fileName, size_t lineNumber, bool useThreadLocal) const {
    Write(TLOG_DEBUG, TStringBuf("DEBUG"), message, fileName, lineNumber, useThreadLocal);
}

void NOxygen::TOxygenLogger::Info(const TString& message, const TStringBuf& fileName, size_t lineNumber, bool useThreadLocal) const {
    Write(TLOG_INFO, TStringBuf("INFO"), message, fileName, lineNumber, useThreadLocal);
}

void NOxygen::TOxygenLogger::Warn(const TString& message, const TStringBuf& fileName, size_t lineNumber, bool useThreadLocal) const {
    Write(TLOG_WARNING, TStringBuf("WARN"), message, fileName, lineNumber, useThreadLocal);
}

void NOxygen::TOxygenLogger::Error(const TString& message, const TStringBuf& fileName, size_t lineNumber, bool useThreadLocal) const {
    Write(TLOG_ERR, TStringBuf("ERROR"), message, fileName, lineNumber, useThreadLocal);
}

void NOxygen::TOxygenLogger::SetLogLevel(ELogPriority level) {
    LogLevel = level;
}

inline void NOxygen::TOxygenLogger::Write(ELogPriority level, const TStringBuf& levelName, const TString& message,
    const TStringBuf& fileName, size_t lineNumber, bool useThreadLocal) const
{
    if (LogLevel < level) {
        return;
    }

    TBuffer buffer;
    FormatMessage(levelName, message, fileName, lineNumber, buffer);
    TLogBackend* backend = useThreadLocal ? GetTlBackend().Get() : nullptr;
    (backend ? backend : Backend.Get())->WriteData(TLogRecord(level, buffer.Data(), buffer.Size()));
}

NOxygen::TOxygenLoggerThreadLocalHolder::TOxygenLoggerThreadLocalHolder(TLogBackendPtr backend) {
    TOxygenLogger::GetInstance().SetThreadLocal(backend);
}

NOxygen::TOxygenLoggerThreadLocalHolder::TOxygenLoggerThreadLocalHolder(const TFsPath& path) {
    TOxygenLogger::GetInstance().SetThreadLocal(path);
}

NOxygen::TOxygenLoggerThreadLocalHolder::~TOxygenLoggerThreadLocalHolder() {
    TOxygenLogger::GetInstance().ResetThreadLocal();
}

TOxygenLoggerThreadLocalInheritHelper::TOxygenLoggerThreadLocalInheritHelper() {
    TLogBackendPtr tl = GetTlBackend();
    if (tl) {
        Backend = TLogBackendPtr(tl.Get(), tl.ReferenceCounter());
        Backend.ReferenceCounter()->Inc();
    }
}

TOxygenLoggerThreadLocalInheritHelper::~TOxygenLoggerThreadLocalInheritHelper() {
}

void TOxygenLoggerThreadLocalInheritHelper::Inject() const {
    if (Backend) {
        TOxygenLogger::GetInstance().SetThreadLocal(Backend);
    }
}
