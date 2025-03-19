#pragma once

#include <library/cpp/logger/global/global.h>

#include <util/system/types.h>
#include <util/system/mem_info.h>

namespace NUtil {
    class TMemoryGuard {
    private:
        const TString Name;
        i64* RSSMetric;
        i64* VMSMetric;
        NMemInfo::TMemInfo StartMemoryInfo;

    public:
        explicit TMemoryGuard(i64* rss, i64* vms, const TString& name)
            : Name(name)
            , RSSMetric(rss)
            , VMSMetric(vms)
            , StartMemoryInfo(NMemInfo::GetMemInfo())
        {
            if (Name) {
                NOTICE_LOG << "Start " << Name << ":" << NLoggingImpl::PrintSystemResources(StartMemoryInfo) << Endl;
            }
        }
        TMemoryGuard(i64& rss, i64& vms, const TString& name = {})
            : TMemoryGuard(&rss, &vms, name)
        {
        }
        TMemoryGuard(i64& rss, const TString& name = {})
            : TMemoryGuard(&rss, nullptr, name)
        {
        }
        TMemoryGuard(const TString& name)
            : TMemoryGuard(nullptr, nullptr, name)
        {
        }

        ~TMemoryGuard() {
            const NMemInfo::TMemInfo& finishMemoryInfo = NMemInfo::GetMemInfo();
            i64 deltaRSS = finishMemoryInfo.RSS - StartMemoryInfo.RSS;
            i64 deltaVMS = finishMemoryInfo.VMS- StartMemoryInfo.VMS;
            if (RSSMetric) {
                *RSSMetric = deltaRSS;
            }
            if (VMSMetric) {
                *VMSMetric = deltaVMS;
            }
            if (Name) {
                NOTICE_LOG << "Finish " << Name << ":" << NLoggingImpl::PrintSystemResources(finishMemoryInfo) << "; " << Sprintf("rss_delta: %0.3fMb, vms_delta: %0.3fMb", 1.0 * deltaRSS / (1024 * 1024), 1.0 * deltaVMS / (1024 * 1024)) << Endl;
            }
        }
    };
}
