#pragma once
#include "default.h"

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            class TNoBalancingPolicy: public TDefaultBalancingPolicy {
            private:
                static TFactory::TRegistrator<TNoBalancingPolicy> Registrator;
            protected:
                virtual bool DoCalculateObjectFeaturesImpl(const TVector<TBalancingObject*>& objects) const override {
                    for (auto&& i : objects) {
                        i->SetBalancingWeight(100).SetBalancingPriority(1);
                    }
                    return true;
                }
            public:
                static TString GetTypeName() {
                    return "no_balancing_policy";
                }

                virtual TString GetClassName() const override {
                    return GetTypeName();
                }
            };
        }
    }
}
