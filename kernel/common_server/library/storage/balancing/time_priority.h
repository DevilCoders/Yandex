#pragma once
#include "default.h"
#include "time_weight.h"

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            class TTimePriorityBalancingPolicy: public TTimeWeightBalancingPolicy {
            private:
                using TBase = TTimeWeightBalancingPolicy;
                static TFactory::TRegistrator<TTimePriorityBalancingPolicy> Registrator;
                CS_ACCESS(TTimePriorityBalancingPolicy, TDuration, ToleranceGap, TDuration::MilliSeconds(5));
            protected:
                virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;
                virtual bool DoCalculateObjectFeaturesImpl(const TVector<TBalancingObject*>& objects) const override;
            public:
                static TString GetTypeName() {
                    return "time_priority_balancing";
                }

                virtual TString GetClassName() const override {
                    return GetTypeName();
                }
            };
        }
    }
}
