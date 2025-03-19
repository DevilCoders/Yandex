#pragma once
#include "feature_description.h"
#include <util/generic/utility.h>
#include <cstddef>
#include <cmath>

namespace NStringMatchTracker {

    class TNormalizerQueryLen {
    public:
        static float Y_FORCE_INLINE Calc(size_t matchSize, size_t qLen, size_t /*docLen*/) {
            if (qLen == 0) {
                return 0.f;
            }
            return ClampVal<float>(float(matchSize) / qLen, 0.f, 1.f);
        }

        static ENormalizer Name() {
            return ENormalizer::QueryLen;
        }
    };

    class TNormalizerDocLen {
    public:
        static float Y_FORCE_INLINE Calc(size_t matchSize, size_t /*qLen*/, size_t docLen)  {
            if (docLen == 0) {
                return 0.f;
            }
            return ClampVal<float>(float(matchSize) / docLen, 0.f, 1.f);
        }

        static ENormalizer Name() {
            return ENormalizer::DocLen;
        }
    };

    class TNormalizerMaxLen {
    public:
        static float Y_FORCE_INLINE Calc(size_t matchSize, size_t qLen, size_t docLen) {
            if (Max(qLen, docLen) == 0) {
                return 0.f;
            }
            return float(matchSize) / Max(qLen, docLen);
        }

        static ENormalizer Name() {
            return ENormalizer::MaxLen;
        }
    };

    class TNormalizerSumLen {
    public:
        static float Y_FORCE_INLINE Calc(size_t matchSize, size_t qLen, size_t docLen) {
            if (qLen * docLen == 0) {
                return 0.f;
            }
            return float(matchSize) / (qLen + docLen);
        }

        static ENormalizer Name() {
            return ENormalizer::SumLen;
        }
    };

    class TNormalizerSimilarityFixed {
    public:
        static float Y_FORCE_INLINE Calc(size_t matchSize, size_t qLen, size_t docLen) {
            if (qLen * docLen == 0) {
                return 0.f;
            }
            const float similarity = float(matchSize) / (sqrtf(float(qLen)) * sqrtf(float(docLen)));
            return similarity / (similarity + 0.333f);
        }

        static ENormalizer Name() {
            return ENormalizer::SimilarityFixed;
        }
    };
}
