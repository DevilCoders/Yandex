#pragma once

#include <util/string/cast.h>

namespace NStringMatchTracker {

    enum class ECalcer {
        None               = 0,
        LcsCalcer          = 1,
        QueryTrigramCalcer = 2,
        DocTrigramCalcer   = 3
    };

    enum class ENormalizer {
        None               = 0,
        QueryLen           = 1,
        DocLen             = 2,
        MaxLen             = 3,
        SumLen             = 4,
        SimilarityFixed    = 5,
    };

    struct TFeatureId {
        ECalcer Calcer;
        ENormalizer Normalizer;
    };

    struct TFeatureDescription {
        TString FullName() const {
            return ToString(FeatureId.Calcer) + ToString(FeatureId.Normalizer);
        }

        TFeatureId FeatureId;
        float Value;
    };
}
