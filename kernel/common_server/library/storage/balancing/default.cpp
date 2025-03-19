#include "default.h"
#include <util/random/random.h>

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            const TBalancingObject* TDefaultBalancingPolicy::DoChooseObject(const TVector<const TBalancingObject*>& objects) const {
                TMap<ui64, ui64> weightsByPriority;
                for (auto&& i : objects) {
                    weightsByPriority[i->GetBalancingPriority()] += i->GetBalancingWeight();
                }
                const ui64 marker = RandomNumber<ui64>(weightsByPriority.rbegin()->second);
                ui64 currentWeight = 0;
                for (auto&& i : objects) {
                    if (i->GetBalancingPriority() != weightsByPriority.rbegin()->first) {
                        continue;
                    }
                    currentWeight += i->GetBalancingWeight();
                    if (currentWeight >= marker) {
                        return i;
                    }
                }
                return nullptr;
            }

            bool TDefaultBalancingPolicy::DoCalculateObjectFeatures(const TVector<TBalancingObject*>& objects) const {
                if (!!GetExternalSettings()) {
                    TString prefix = "pg_balancing.";
                    if (DCLocalCode) {
                        prefix += DCLocalCode + ".";
                    }
                    prefix += GetDBName() + ".weight.";
                    for (auto&& i : objects) {
                        const TString DCCode = NUtil::DetectDatacenterCode(i->GetHostName());
                        const ui64 resultDef = (DCLocalCode == DCCode) ? 100000 : 1;
                        const ui64 w = GetExternalSettings()->GetValueDef(prefix + i->GetHostName(), GetExternalSettings()->GetValueDef(prefix + DCCode, resultDef));
                        i->SetSettingsWeight(w);
                    }
                }
                return DoCalculateObjectFeaturesImpl(objects);
            }

        }
    }
}
