#pragma once
#include "default.h"

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            class TTimeWeightBalancingPolicy: public TDefaultBalancingPolicy {
            private:
                static TFactory::TRegistrator<TTimeWeightBalancingPolicy> Registrator;
            protected:
                virtual bool DoCalculateObjectFeaturesImpl(const TVector<TBalancingObject*>& objects) const override;
            public:
                static TString GetTypeName() {
                    return "time_weight_balancing";
                }

                virtual TString GetClassName() const override {
                    return GetTypeName();
                }
            };
        }
    }
}
