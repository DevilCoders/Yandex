#pragma once

#include <util/datetime/base.h>
#include <library/cpp/logger/global/global.h>
#include <util/system/rusage.h>

template <bool Additive = false, ELogPriority LogLevel = TLOG_DEBUG, bool WithMemory = false>
class TTimeGuardImpl {
private:
    ui64* Storage = nullptr;
    TDuration* StorageDuration = nullptr;
    const TInstant Start;
    const TString Comment;
public:
    TTimeGuardImpl(ui64& storageLink)
        : Storage(&storageLink)
        , Start(Now()) {

    }

    TTimeGuardImpl(const TString& comment)
        : Start(Now())
        , Comment(comment)
    {
        PrintStart();
    }

    TTimeGuardImpl(TDuration* storageLink = nullptr, const TString& comment = TString())
        : StorageDuration(storageLink)
        , Start(Now())
        , Comment(comment)
    {
        if (!storageLink && comment.empty()) {
            CHECK_WITH_LOG(!Additive);
        }
        PrintStart();
    }

    TTimeGuardImpl(TDuration& storageLink, const TString& comment = TString())
        : TTimeGuardImpl(&storageLink, comment)
    {
    }

    ui64 Release() {
        TDuration result = Now() - Start;
        PrintFinish(result);
        if (StorageDuration) {
            if (Additive) {
                *StorageDuration += result;
            } else {
                *StorageDuration = result;
            }
        } else if (Storage) {
            if (Additive) {
                *Storage += result.MicroSeconds();
            } else {
                *Storage = result.MicroSeconds();
            }
        }
        return result.MicroSeconds();
    }

    ~TTimeGuardImpl() {
        Release();
    }
private:
    void PrintStart() const {
        if (!Comment.empty()) {
            TEMPLATE_LOG(LogLevel) << Comment << ": START " << (WithMemory ? ("[MEMORY:" + ::ToString(TRusage::GetCurrentRSS() / 1024 / 1024) + " Mb]") : "") << Endl;
        }
    }
    void PrintFinish(const TDuration& result) const {
        if (!Comment.empty()) {
            TEMPLATE_LOG(LogLevel) << Comment << ": FINISH(" << result.ToString() << ") " << (WithMemory ? ("[MEMORY:" + ::ToString(TRusage::GetCurrentRSS() / 1024 / 1024) + " Mb]") : "") << Endl;
        }
    }
};

using TTimeGuard = TTimeGuardImpl<false, ELogPriority::TLOG_DEBUG>;

using TTimeGuardAdditive = TTimeGuardImpl<true, ELogPriority::TLOG_DEBUG>;
