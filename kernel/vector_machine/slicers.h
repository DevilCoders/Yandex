#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NVectorMachine {
    class TAllSlicer {
    public:
        TAllSlicer(const float thresholdValue)
        {
            Y_UNUSED(thresholdValue);
        }

        std::pair<size_t, size_t> GetBeginAndSize(const TVector<float>& thresholdValues, const size_t scoreSize) const {
            Y_UNUSED(thresholdValues);
            return {0, scoreSize};
        }
    };

    class TLessSlicer {
    public:
        TLessSlicer(const float thresholdValue)
            : ThresholdValue_(thresholdValue)
        {
        }

        std::pair<size_t, size_t> GetBeginAndSize(const TVector<float>& thresholdValues, const size_t scoreSize) const {
            Y_ENSURE(thresholdValues.size() == scoreSize);
            const size_t size = std::distance(thresholdValues.begin(), LowerBound(thresholdValues.begin(), thresholdValues.end(), ThresholdValue_));
            return {0, size};
        }

    private:
        float ThresholdValue_;
    };

    class TGreaterEqualSlicer {
    public:
        TGreaterEqualSlicer(const float thresholdValue)
            : ThresholdValue_(thresholdValue)
        {
        }

        std::pair<size_t, size_t> GetBeginAndSize(const TVector<float>& thresholdValues, const size_t scoreSize) const {
            Y_ENSURE(thresholdValues.size() == scoreSize);
            const size_t lowerBoundIndex = std::distance(thresholdValues.begin(), LowerBound(thresholdValues.begin(), thresholdValues.end(), ThresholdValue_));
            return {lowerBoundIndex, thresholdValues.size() - lowerBoundIndex};
        }

    private:
        float ThresholdValue_;
    };
}
