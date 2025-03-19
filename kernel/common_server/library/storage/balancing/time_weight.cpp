#include "time_weight.h"

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            TTimeWeightBalancingPolicy::TFactory::TRegistrator<TTimeWeightBalancingPolicy> TTimeWeightBalancingPolicy::Registrator(TTimeWeightBalancingPolicy::GetTypeName());

            bool TTimeWeightBalancingPolicy::DoCalculateObjectFeaturesImpl(const TVector<TBalancingObject*>& objects) const {
                TDuration pingSum;
                for (auto&& i : objects) {
                    pingSum += i->GetPingDuration();
                }
                for (auto&& i : objects) {
                    double kff;
                    if (!pingSum) {
                        kff = 1;
                    } else if (!i->GetPingDuration()) {
                        kff = 1000000;
                    } else {
                        kff = (double)pingSum.GetValue() / (double)i->GetPingDuration().GetValue();
                    }
                    i->SetBalancingWeight(i->GetSettingsWeight() * kff);
                }
                return true;
            }

        }
    }
}
