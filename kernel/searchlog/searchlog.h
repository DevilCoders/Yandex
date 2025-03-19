#pragma once

#include <library/cpp/logger/all.h>
#include <library/cpp/logger/filter.h>

#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/stream/debug.h>

class TSearchLog: public TLog {
    TString LogPath;

private:
    /// to ensure proper order of construction/destruction
    class THelper {
    public:
        inline THelper(const TString& fname)
            : F_(MakeHolder<TFileLogBackend>(fname))
        { }

    protected:
        TReopenLogBackend F_;
    };

    class TBackend: public THelper, public TThreadedLogBackend {
    private:
        size_t QueueLen;

    public:
        inline TBackend(const TString& fname, size_t queueLen)
            : THelper(fname)
            , TThreadedLogBackend(&F_, queueLen)
            , QueueLen(queueLen)
        { }

        void WriteData(const TLogRecord& rec) override {
            try {
                TThreadedLogBackend::WriteData(rec);
            } catch (const yexception&) {
                // SEARCH-413 Positive queue length means errors should be ignored
                if (!QueueLen)
                    throw;
                Cdbg << "WriteData exception handled: " << CurrentExceptionMessage() << Endl;
            }
        }

        ~TBackend() override {
        }
    };

private:
    TAutoPtr<TLogBackend> CreateBackend(const TString& fname, ELogPriority priority, size_t queueLen, bool skipEmptyFilename) {
        if (!fname) {
            if (skipEmptyFilename) {
                return MakeHolder<TNullLogBackend>().Release();
            }

            Y_FAIL("Empty log filename specified");
        }

        auto mainBackend = MakeHolder<TBackend>(fname, queueLen);
        if (priority >= LOG_MAX_PRIORITY) {
            return mainBackend.Release();
        } else {
            auto filtered = MakeHolder<TFilteredLogBackend>(std::move(mainBackend), priority);
            return filtered.Release();
        }
    }

public:
    inline TSearchLog()
        : TLog()
    { }

    inline TSearchLog(const TString& fname, size_t queueLen = 0)
        : TLog(CreateBackend(fname, LOG_MAX_PRIORITY, queueLen, false))
        , LogPath(fname)
    { }

    inline explicit TSearchLog(const TString& fname, ELogPriority priority, size_t queueLen = 0)
        : TLog(CreateBackend(fname, priority, queueLen, true))
        , LogPath(fname)
    { }

    const TString& GetLogPath() const {
        return LogPath;
    }
};
