#include "annotation_accumulator_motor.h"

#include <util/generic/utility.h>
#include <util/generic/ymath.h>
#include <util/system/yassert.h>

namespace NTextMachine {
namespace NCore {
namespace NAnnotationAccumulatorParts {
    Y_CONST_FUNCTION float CalcPositionAdjust(TQueryWordId queryPosition, TBreakWordId annotationPosition)
    {
        static_assert(sizeof(TBreakWordId) <= 2, "TBreakWordId is too large");
        static_assert(sizeof(TQueryWordId) <= 2, "TQueryWordId is too large");

        if (queryPosition == annotationPosition) {
            return 1.0f;
        }

        const ui16 dist = (queryPosition > annotationPosition)
            ? 1 + queryPosition - annotationPosition
            : 1 + annotationPosition - queryPosition;

        if (dist > 255) {
            return 0.0f;
        }
        return 1.0f / static_cast<float>(dist * dist);
    }

    Y_CONST_FUNCTION float CalcProximity(TBreakWordId leftPosition, TBreakWordId rightPosition)
    {
        static_assert(sizeof(TBreakWordId) <= 2, "TBreakWordId is too large");

        if (leftPosition >= rightPosition) {
            return 1.0f;
        }
        const ui16 dist = rightPosition - leftPosition;
        if (dist > 255) {
            return 0.0f;
        }
        return 1.0f / static_cast<float>(dist * dist);
    }

    Y_CONST_FUNCTION int CalcOldWordDistance(TBreakWordId left, TBreakWordId right) {
        return (static_cast<int>(right)) - (left);
    }
    Y_CONST_FUNCTION float CalcOldBclmDist(TBreakId leftBrk, TBreakWordId leftWrd, TBreakId rightBrk, TBreakWordId rightWrd) {
        constexpr float BCLMLITE_MIN_PROXIMITY = 1.f / (65.f * 65.f);
        constexpr TBreakWordId BCLMLITE_MAX_WRD_DISTANCE = 66;
        if (leftBrk == rightBrk) {
            if (rightWrd <= leftWrd + 1) {
                return 1.f;
            }
            Y_ASSERT(leftWrd <= rightWrd);
            Y_ASSERT(rightWrd - leftWrd < BCLMLITE_MAX_WRD_DISTANCE);
            return 1.f / static_cast<float>((rightWrd - leftWrd) * (rightWrd - leftWrd));
        }
        return BCLMLITE_MIN_PROXIMITY;
    }
} // NAnnotationAccumulatorParts
} // NCore
} // NTextMachine
