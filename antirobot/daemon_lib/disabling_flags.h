#pragma once

#include "service_param_holder.h"

#include <library/cpp/deprecated/atomic/atomic.h>

namespace NAntiRobot {
    class TDisablingFlags {
    public:
        bool IsNeverBlockForService(EHostType service) const {
            return AtomicGet(NeverBlock) || AtomicGet(NeverBlockByService.GetByService(service));
        }

        bool IsNeverBanForService(EHostType service) const {
            return AtomicGet(NeverBan) || AtomicGet(NeverBanByService.GetByService(service));
        }

        bool IsStopFuryForAll() const {
            return AtomicGet(StopFury);
        }

        bool IsStopFuryPreprodForAll() const {
            return AtomicGet(StopFuryPreprod);
        }

        bool IsStopYqlForService(EHostType service) const {
            return AtomicGet(StopYqlForAll) || AtomicGet(StopYql.GetByService(service));
        }

        bool IsStopDiscoveryForAll() const {
            return AtomicGet(StopDiscoveryForAll);
        }

        bool IsDisableCatboostWhitelist(EHostType service) const {
            return AtomicGet(DisableCatboostWhitelistForAll) || AtomicGet(DisableCatbostWhitelist.GetByService(service));
        }

    public:
        TAtomic NeverBlock = 0;
        TServiceParamHolder<TAtomic> NeverBlockByService;
        TAtomic NeverBan = 0;
        TServiceParamHolder<TAtomic> NeverBanByService;
        TAtomic StopFury = 0;
        TAtomic StopFuryPreprod = 0;
        TServiceParamHolder<TAtomic> StopYql;
        TAtomic StopYqlForAll = 0;
        TAtomic StopDiscoveryForAll = 0;
        TAtomic DisableCatboostWhitelistForAll = 0;
        TServiceParamHolder<TAtomic> DisableCatbostWhitelist;
    };
}
