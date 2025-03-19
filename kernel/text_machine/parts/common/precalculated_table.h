#pragma once

#include <library/cpp/vec4/vec4.h>

#include <util/generic/ymath.h>

#include <array>

namespace NTextMachine {
namespace NCore {
    template <size_t TableSize, typename FunctionToInterpolate>
    class TPrecalculatedTable {
        std::array<float, TableSize + 1> Values;
        float Scale = NAN;
        TVec4f ScaleVec;

    public:
        Y_FORCE_INLINE float GetValue(float input) const {
            Y_VERIFY_DEBUG(input >= 0, "%f", input);
            float fIndex = input * Scale;
            size_t iPart = static_cast<size_t>(fIndex);
            if (iPart < TableSize) {
                float fPart = fIndex - (float)iPart;
                return Values[iPart] + (Values[iPart + 1] - Values[iPart]) * fPart;
            } else {
                return Values[TableSize];
            }
        }

        Y_FORCE_INLINE TVec4f GetValue(TVec4f input) const {
            TVec4f fIndex = input * ScaleVec;
            TVec4<int> iPart = fIndex.Truncate();
            int iParts[4];
            iPart.Store(iParts);

            float x0, x1, x2, x3;
            float y0, y1, y2, y3;

#define PREPARE(i) \
    Y_VERIFY_DEBUG(iParts[i] >= 0, "%d", iParts[i]); \
    if (size_t(iParts[i]) < TableSize) { \
        x##i = Values[iParts[i]]; \
        y##i = Values[iParts[i] + 1]; \
    } else { \
        x##i = Values[TableSize]; \
        y##i = x##i; \
    }

            PREPARE(0)
            PREPARE(1);
            PREPARE(2);
            PREPARE(3);

#undef PREPARE

            TVec4f fX(x0, x1, x2, x3);
            TVec4f fY(y0, y1, y2, y3);

            return fX + (fY - fX) * (fIndex - iPart.ToFloat());
        }

        Y_FORCE_INLINE float GetValueNotInterpolated(float input) const {
            Y_ASSERT(input >= 0);
            float fIndex = input * Scale;
            size_t iPart = (size_t)fIndex;
            if (iPart < TableSize) {
                return Values[iPart];
            } else {
                return Values[TableSize];
            }
        }

        TPrecalculatedTable(float maxValue) {
            FunctionToInterpolate functionToInterpolate;
            static_assert(TableSize > 0, "");
            Y_ASSERT(maxValue > 0);
            Scale = float(TableSize) / maxValue;
            ScaleVec = TVec4f(Scale, Scale, Scale, Scale);
            for (size_t i = 0; i <= TableSize; ++i) {
                float x = i / Scale;
                Values[i] = functionToInterpolate(x);
            }
        }
    };

};
};
