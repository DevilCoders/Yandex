#include <kernel/text_machine/parts/common/precalculated_table.h>
#include <kernel/text_machine/parts/common/types.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/random.h>
#include <util/generic/xrange.h>

using namespace NTextMachine;
using namespace NTextMachine::NCore;

namespace {
    struct MergeLogFunctor {
        double operator()(double x) {
            return log(1.0f + x);
        }
    };
    TPrecalculatedTable<8192, MergeLogFunctor> MergeLogPrecalculatedTable(256.0f);
};


Y_UNIT_TEST_SUITE(TPrecalculatedTableTest) {
    Y_UNIT_TEST(TestSimple) {
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(0), log(1.0f + 0.0f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(0.25f), log(1.0f + 0.25f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(0.5f), log(1.0f + 0.5f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(1.0f), log(1.0f + 1.0f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(2.0f), log(1.0f + 2.0f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(16.0f), log(1.0f + 16.0f), FloatEpsilon);

        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValueNotInterpolated(0), log(1.0f + 0.0f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValueNotInterpolated(0.25f), log(1.0f + 0.25f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValueNotInterpolated(1.0f), log(1.0f + 1.0f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValueNotInterpolated(16.0f), log(1.0f + 16.0f), FloatEpsilon);
    }

    Y_UNIT_TEST(TestInterpolation) {
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(0.5f / 32.0f), log(1.0f) * 0.5f + log(1.0f + 1.0f / 32.0f) * 0.5f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(0.5f / 32.0f + 3.0f / 32.0f),
            log(1.0f + 3.0f / 32.0f) * 0.5f + log(1.0f + 4.0f / 32.0f) * 0.5f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(0.3f / 32.0f + 7.0f / 32.0f),
            log(1.0f + 7.0f / 32.0f) * 0.7f + log(1.0f + 8.0f / 32.0f) * 0.3f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(0.7f / 32.0f + 7.0f / 32.0f),
            log(1.0f + 7.0f / 32.0f) * 0.3f + log(1.0f + 8.0f / 32.0f) * 0.7f, FloatEpsilon);

        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValueNotInterpolated(0.5f / 32.0f), log(1.0f) * 0.5f, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValueNotInterpolated(0.7f / 32.0f + 7.0f / 32.0f),
            log(1.0f + 7.0f / 32.0f), FloatEpsilon);
    }

    Y_UNIT_TEST(TestOverflow) {
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(256.0f), log(1.0f + 256.0f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValue(256.0f), MergeLogPrecalculatedTable.GetValue(1000.0f), FloatEpsilon);

        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValueNotInterpolated(256.0f), log(1.0f + 256.0f), FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(MergeLogPrecalculatedTable.GetValueNotInterpolated(256.0f), MergeLogPrecalculatedTable.GetValueNotInterpolated(1000.0f), FloatEpsilon);
    }

    Y_UNIT_TEST(TestVec4f) {
        float values[] = {
            0.0f, 1.0f, 2.0f, 3.0f,
            0.25f, 0.5f, 0.75f, 1.0f,
            250.0f, 0.25f, 2500.0f, 0.001f
        };
        const size_t numValues = Y_ARRAY_SIZE(values);

        for (size_t i : xrange<size_t>(0, numValues, 4)) {
            TVec4f vx;
            vx.Load(values + i);
            TVec4f vy(MergeLogPrecalculatedTable.GetValue(values[i + 0]),
                MergeLogPrecalculatedTable.GetValue(values[i + 1]),
                MergeLogPrecalculatedTable.GetValue(values[i + 2]),
                MergeLogPrecalculatedTable.GetValue(values[i + 3]));
            UNIT_ASSERT_EQUAL(MergeLogPrecalculatedTable.GetValue(vx).AsInt(), vy.AsInt());
        }
    }

    Y_UNIT_TEST(TestVec4fRandom) {
        const size_t numValues = 1000;
        for (size_t i : xrange(numValues)) {
            Y_UNUSED(i);
            float x0 = RandomNumber<float>();
            float x1 = RandomNumber<float>() * 10;
            float x2 = RandomNumber<float>() * 100;
            float x3 = RandomNumber<float>() * 10000;
            TVec4f vx(x0, x1, x2, x3);
            TVec4f vy(MergeLogPrecalculatedTable.GetValue(x0),
                MergeLogPrecalculatedTable.GetValue(x1),
                MergeLogPrecalculatedTable.GetValue(x2),
                MergeLogPrecalculatedTable.GetValue(x3));
            UNIT_ASSERT_EQUAL(MergeLogPrecalculatedTable.GetValue(vx).AsInt(), vy.AsInt());
        }
    }
}
