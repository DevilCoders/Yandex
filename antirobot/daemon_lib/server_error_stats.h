#pragma once

#include "stat.h"
#include "service_param_holder.h"

namespace NAntiRobot {

struct TServerErrorStats {
    enum class EHistogram {
        ServerError  /* "server_error" */,
        Count
    };

    TCategorizedHistograms<
        std::atomic<size_t>, EHistogram,
        EHostType
    > Histograms;

    void Update(const TRequest& req);
    void PrintStats(TStatsWriter& out) const;
};

struct TServerErrorFlags {
    TServiceParamHolder<TAtomic> ServiceDisable;
    TAtomic Enable = 0;

    void EnableErrorServiceDisable(EHostType service) {
        AtomicSet(ServiceDisable.GetByService(service), 1);
    }

    void DisableErrorServiceDisable(EHostType service) {
        AtomicSet(ServiceDisable.GetByService(service), 0);
    }
};

} // namespace NAntiRobot
