#pragma once

#include "stat.h"
#include "rps.h"
#include "histogram.h"

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>

namespace NStat {
    // Specialized per-request stats for separate clients
    class TRequestStat: public TStat {
    public:
        TRequestStat(TDuration maxPeriod)
            : TStat(false) // no memory stat
            , RpsCounter(maxPeriod)
        {
        }

        void InitHistogram(TVector<TDurationHistogram::TColumnGroup>&& groups) {
            TimeHist.Reset(new TDurationHistogram(std::move(groups)));
        }

        virtual void RegisterRequest(const TTimeInfo& timing, const TString& clientId);

        // Errors can be triggered before actual request processing starts,
        // so they are registered separately
        virtual void RegisterError(const TStringBuf& type);

        virtual void ToProto(TStatProto& proto) const override;

        NSc::TValue HistogramJson(TDuration timeUnit) const {
            return TimeHist ? TimeHist->ToJson(timeUnit) : NSc::TValue();
        }

    private:
        TRpsCounter RpsCounter;
        THolder<TDurationHistogram> TimeHist;

        THashMap<TString, size_t> Errors; // errors groupped by type
    };

}
