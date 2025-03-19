#include "counter.h"

namespace NCommonProxy {

    TCounter::TCounter(const TString& name)
        : Rate(TDuration::Seconds(2 * Periods.back()), 2 * Periods.back())
        , Name(name)
    {}

    ui64 TCounter::GetCount() const {
        return AtomicGet(Processed);
    }

    ui32 TCounter::GetCount(const TDuration& period) const {
        return Rate.Get(Now() - period);
    }

    void TCounter::WriteInfo(NJson::TJsonValue& result) const {
        NJson::TJsonValue& rps = result[Name + "_per_second"];
        TInstant now = Now();
        for (auto i : Periods) {
            rps[ToString(i)] = (float)Rate.Get(now - TDuration::Seconds(i)) / i;
        }
        result[Name + "_count"] = GetCount();
    }
}
