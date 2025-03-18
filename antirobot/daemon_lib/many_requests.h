#pragma once
#include "stat.h"
#include "service_param_holder.h"

namespace NAntiRobot {

struct TManyRequestsStats {
    enum class ECounter {
        ManyRequests        /* "many_requests" */,
        ManyRequestsMobile  /* "many_requests_mobile" */,
        Count
    };

    TCategorizedStats<
        std::atomic<size_t>, ECounter,
        EHostType
    > Counters;

    void Update(const TRequest& req);
    void PrintStats(TStatsWriter& out) const;
};


struct TSuspiciousFlags {
    TServiceParamHolder<TAtomic> ManyRequests;
    TServiceParamHolder<TAtomic> ManyRequestsMobile;
    TServiceParamHolder<TAtomic> Ban;
    TServiceParamHolder<TAtomic> Block;

    void EnableManyRequests(EHostType service) {
        AtomicSet(ManyRequests.GetByService(service), 1);
    }

    void DisableManyRequests(EHostType service) {
        AtomicSet(ManyRequests.GetByService(service), 0);
    }

    void EnableManyRequestsMobile(EHostType service) {
        AtomicSet(ManyRequestsMobile.GetByService(service), 1);
    }

    void DisableManyRequestsMobile(EHostType service) {
        AtomicSet(ManyRequestsMobile.GetByService(service), 0);
    }

    void EnableBan(EHostType service) {
        AtomicSet(Ban.GetByService(service), 1);
    }

    void DisableBan(EHostType service) {
        AtomicSet(Ban.GetByService(service), 0);
    }

    void EnableBlock(EHostType service) {
        AtomicSet(Block.GetByService(service), 1);
    }

    void DisableBlock(EHostType service) {
        AtomicSet(Block.GetByService(service), 0);
    }
};

} // namespace NAntiRobot
