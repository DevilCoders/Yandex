#pragma once
#include "abstract.h"

namespace NCS {
    namespace NStorage {
        namespace NBalancing {
            class TDefaultBalancingPolicy: public IBalancingPolicy {
            protected:
                virtual bool DoCalculateObjectFeaturesImpl(const TVector<TBalancingObject*>& objects) const = 0;
                virtual const TBalancingObject* DoChooseObject(const TVector<const TBalancingObject*>& objects) const override;
                virtual bool DoCalculateObjectFeatures(const TVector<TBalancingObject*>& objects) const override final;

            };
        }
    }
}
