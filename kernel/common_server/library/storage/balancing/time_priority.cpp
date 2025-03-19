#include "time_priority.h"
#include <kernel/common_server/util/json_processing.h>

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            TTimePriorityBalancingPolicy::TFactory::TRegistrator<TTimePriorityBalancingPolicy> TTimePriorityBalancingPolicy::Registrator(TTimePriorityBalancingPolicy::GetTypeName());

            bool TTimePriorityBalancingPolicy::DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                if (!TJsonProcessor::Read(jsonInfo, "tolerance_gap", ToleranceGap)) {
                    return false;
                }
                return true;
            }

            bool TTimePriorityBalancingPolicy::DoCalculateObjectFeaturesImpl(const TVector<TBalancingObject*>& objects) const {
                TDuration minPing = TDuration::Max();
                for (auto&& i : objects) {
                    if (minPing > i->GetPingDuration()) {
                        minPing = i->GetPingDuration();
                    }
                }
                TVector<TBalancingObject*> pObjects;
                for (auto&& i : objects) {
                    if (minPing + ToleranceGap >= i->GetPingDuration()) {
                        i->SetBalancingPriority(1);
                        pObjects.emplace_back(i);
                    } else {
                        i->SetBalancingPriority(0);
                    }
                }
                return TBase::DoCalculateObjectFeaturesImpl(pObjects);
            }

        }
    }
}
