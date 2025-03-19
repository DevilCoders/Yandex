#pragma once

#include <alice/protos/api/nlu/generated/features.pb.h>
#include <alice/protos/api/nlu/feature_container.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/yexception.h>

namespace NAlice::NNluFeatures {

    inline TMaybe<float> GetFeatureMaybe(const TFeatureContainer& container, const ENluFeature feature) {
        const int featureIdx = feature;
        Y_ENSURE(ENluFeature_IsValid(feature), "feature enum is not valid: " << featureIdx);

        if (container.GetFeatures().size() <= featureIdx) {
            // can happen if begemot uses old config without new features
            return Nothing();
        }
        return container.GetFeatures()[feature];
    }

    inline float GetFeature(
        const TFeatureContainer& container, const ENluFeature feature, const float defaultVal = 0.0F
    ) {
        const TMaybe<float> featureValue = GetFeatureMaybe(container, feature);

        if (featureValue.Empty()) {
            return defaultVal;
        }

        return featureValue.GetRef();
    }

}  // namespace NAlice::NNluFeatures
