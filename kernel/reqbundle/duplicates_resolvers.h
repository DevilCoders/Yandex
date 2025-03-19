#pragma once

#include "reqbundle.h"

#include <util/generic/algorithm.h>

namespace NReqBundle {
namespace NDetail {
    class IDuplicatesResolver {
    public:
        virtual float GetUpdatedValue(float oldValue, float newValue, const TFacetId& id) const = 0;
        //final weight in case of no collisions
        virtual float GetNormalizedValue(float value, const TFacetId& /*id*/) const {
            return value;
        }
        virtual ~IDuplicatesResolver() {
        }
    };

    class TTakeMaxResolver: public IDuplicatesResolver {
    public:
        float GetUpdatedValue(float oldValue, float newValue, const TFacetId& /*id*/) const override {
            return ::Max(oldValue, newValue);
        }
    };

    class TSoftSignResolver: public NReqBundle::NDetail::IDuplicatesResolver {
    public:
        const float Scale = 2.0;
        const float Bias = 0.5;

        float SoftSign(float x) const {
            return Min(x / (Scale * fabs(x) + 1.0) + Bias, 1.0);
        }

        float InverseSoftSign(float y) const {
            if (y > Bias) { // x is positive
                return (y - Bias) / (1.0 - Scale * y + Scale * Bias);
            }
            return (y - Bias) / (1.0 + Scale * y - Scale * Bias);
        }

        float GetUpdatedValue(float oldValue, float newValue, const TFacetId& /*id*/) const override {
            return SoftSign(newValue + InverseSoftSign(oldValue));
        }
        float GetNormalizedValue(float value, const TFacetId& /*id*/) const override {
            return SoftSign(value);
        }
    };

    class TByExpansionTypeDuplicatesResolver: public IDuplicatesResolver {
    private:
        const IDuplicatesResolver* DefaultResolver = nullptr;
        using TExpansionType = NLingBoost::TExpansionStruct::EType;
        THashMap<TExpansionType, const IDuplicatesResolver*> Resolvers;
        const IDuplicatesResolver* GetResolver(const TExpansionType& id) const;
    public:
        TByExpansionTypeDuplicatesResolver(const IDuplicatesResolver* defaultResolver)
            : DefaultResolver(defaultResolver)
        {
            Y_ASSERT(defaultResolver);
        }

        void SetResolver(TExpansionType type, const IDuplicatesResolver* resolver) {
            Y_ASSERT(resolver);
            Resolvers[type] = resolver;
        }

        float GetUpdatedValue(float oldValue, float newValue, const TFacetId& id) const override;
        float GetNormalizedValue(float value, const TFacetId& id) const override;
    };
}
}
