#pragma once
#include "default.h"

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            class TConfiguredBalancingPolicy: public TDefaultBalancingPolicy {
            private:
                static TFactory::TRegistrator<TConfiguredBalancingPolicy> Registrator;
            protected:
                virtual bool DoCalculateObjectFeaturesImpl(const TVector<TBalancingObject*>& objects) const override {
                    for (auto&& i : objects) {
                        i->SetBalancingWeight(i->GetSettingsWeight());
                    }
                    return true;
                }
            public:
                static TString GetTypeName() {
                    return "configured_balancing";
                }

                virtual TString GetClassName() const override {
                    return GetTypeName();
                }
            };
        }
    }
}
