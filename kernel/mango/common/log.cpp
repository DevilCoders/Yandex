#include "log.h"

#include "tools.h"

#include <library/cpp/logger/backend.h>
#include <library/cpp/logger/file.h>

#include <util/datetime/base.h>
#include <util/datetime/systime.h>
#include <util/folder/path.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/printf.h>
#include <util/system/backtrace.h>
#include <util/system/rwlock.h>

#if defined(_win_)
    #include <io.h>
#endif

static const char* LOG_PRIORITY_NAMES[] = { "EMERG", "ALERT", "CRIT", "ERR", "WARNING", "NOTICE", "INFO", "DEBUG" };

namespace NMango {

TString TruncateForLogging(const TString &str) {
    TString result(str);
    static const size_t maxSize = 10 * 1024;
    if (result.size() > maxSize) {
        result.resize(maxSize);
        result += "[...]";
    }
    return result;
}

static TString GetPreamble(const TLogRecord &rec) {
    return Sprintf("%s [%s]\t", NMango::GetTimeFormatted("[%Y-%m-%d, %H:%M:%S]").data(), LOG_PRIORITY_NAMES[static_cast<int>(rec.Priority)]);
}

class TTwoWayStreamBackend : public TLogBackend
{
    IOutputStream &Out, &Err;
    const ELogPriority MaxPriority;
    bool ThrowOnError;
public:
    TTwoWayStreamBackend(IOutputStream &out, IOutputStream &err, bool throwOnError, ELogPriority maxPriority)
        : Out(out), Err(err), MaxPriority(maxPriority), ThrowOnError(throwOnError)
    {}

    void WriteData(const TLogRecord &rec) override
    {
        if (rec.Priority <= MaxPriority) {
            IOutputStream &out = rec.Priority <= TLOG_ERR ? Err : Out;
            out << GetPreamble(rec);
            out.Write(rec.Data, rec.Len);
            if (ThrowOnError && rec.Priority <= TLOG_ERR) {
                TString trace;
                TStringOutput sout(trace);
                FormatBackTrace(&sout);
                out.Write(trace);
                out.Flush();
                ythrow yexception() << TString(rec.Data, rec.Len);
            }
            out.Flush();
        }
    }

    void ReopenLog() override {}
};

class TSingleFileBackend : public TLogBackend {
private:
    TFile UnderlyingFile;
    ELogPriority MaxPriority;
    bool ShouldRedirectStandardStreams;
    TRWMutex Lock;

    static constexpr EOpenMode OpenFlags = OpenAlways | WrOnly | ForAppend;

    void RedirectStandardStream(int streamNumber) {
#if defined(_win_)
        FHANDLE stream = (FHANDLE)_get_osfhandle(streamNumber);
#else
        FHANDLE stream = streamNumber;
#endif
        TFileHandle streamHandle(stream);
        TFileHandle logHandle(UnderlyingFile.GetHandle());
        streamHandle.LinkTo(logHandle);
        streamHandle.Release();
        logHandle.Release();
    }

    void RedirectStandardStreams() {
        if (ShouldRedirectStandardStreams) {
            RedirectStandardStream(1);
            RedirectStandardStream(2);
        }
    }

public:
    TSingleFileBackend(const TString &logFile, ELogPriority maxPriority, bool shouldRedirectStandardStreams)
        : UnderlyingFile(logFile, OpenFlags)
        , MaxPriority(maxPriority)
        , ShouldRedirectStandardStreams(shouldRedirectStandardStreams)
    {
        RedirectStandardStreams();
    }

    void WriteData(const TLogRecord &rec) override {
        if (rec.Priority <= MaxPriority) {
            TReadGuard guard(Lock);
            TString preamble(GetPreamble(rec));
            UnderlyingFile.Write(preamble.data(), preamble.size());
            UnderlyingFile.Write(rec.Data, rec.Len);
        }
    }

    void ReopenLog() override {
        TWriteGuard guard(Lock);
        UnderlyingFile.LinkTo(TFile(UnderlyingFile.GetName(), OpenFlags));
        RedirectStandardStreams();
    }

    ~TSingleFileBackend() override {
    }
};

constexpr EOpenMode TSingleFileBackend::OpenFlags;

TMangoLogSettings::TMangoLogSettings()
{
    SetTwoWayBackend(Cout, Cerr);
    Log.SetDefaultPriority(TLOG_INFO);
}

TMangoLogSettings* TMangoLogSettings::Instance()
{
    return Singleton<TMangoLogSettings>(); // TODO: constant singleton
}

TLog& TMangoLogSettings::GetLog()
{
    return Instance()->Log;
}

void TMangoLogSettings::SetKosherBackend(IOutputStream &out, bool throwOnError, ELogPriority maxPriority)
{
    TAutoPtr<TLogBackend> backend(new TTwoWayStreamBackend(out, out, throwOnError, maxPriority));
    Log.ResetBackend(backend);
}

void TMangoLogSettings::SetTwoWayBackend(IOutputStream &out, IOutputStream &err, bool throwOnError, ELogPriority maxPriority)
{
    TAutoPtr<TLogBackend> backend(new TTwoWayStreamBackend(out, err, throwOnError, maxPriority));
    Log.ResetBackend(backend);
}

void TMangoLogSettings::SetFileBackend(const TString &logFile, ELogPriority maxPriority, bool shouldRedirectStandardStreams) {
    TAutoPtr<TLogBackend> backend(new TSingleFileBackend(logFile, maxPriority, shouldRedirectStandardStreams));
    Log.ResetBackend(backend);
}

}
