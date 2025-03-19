#include <kernel/text_machine/parts/accumulators/window_accumulator.h>
#include <kernel/text_machine/parts/common/weights.h>
#include <kernel/text_machine/parts/common/types.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NTextMachine;
using namespace NTextMachine::NCore;

Y_UNIT_TEST_SUITE(TMovingWindowTest) {
    TMemoryPool Pool{32 << 10};

    Y_UNIT_TEST(TestSimple) {
        TMovingWindow movingWindow(10, 20);
        TWeightsHolder weights;
        weights.Init(Pool, 3, EStorageMode::Empty);
        weights.Add(0.5f);
        weights.Add(0.19f);
        weights.Add(0.31f);
        movingWindow.Init(Pool, 3, weights);

        for (size_t counter = 0; counter < 2; ++counter) {
            movingWindow.NewDoc();

            movingWindow.AddHit(1, 0);
            movingWindow.AddHit(2, 1);

            movingWindow.AddHit(0, 11);
            movingWindow.AddHit(1, 12);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcm(), 0.69, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcmAttenuation(), 0.69 * 20.0 / 22.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcmFullAttenuation(), 0.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm95Attenuation(), 0.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm80Attenuation(), 0.0, FloatEpsilon);

            movingWindow.AddHit(0, 22);
            movingWindow.AddHit(2, 23);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcm(), 0.81, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcmAttenuation(), 0.69 * 20.0 / 22.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcmFullAttenuation(), 0.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm95Attenuation(), 0.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm80Attenuation(), 20.0 / 33.0, FloatEpsilon);
            movingWindow.AddHit(1, 24);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcm(), 1.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcmAttenuation(), 0.69 * 20.0 / 22.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcmFullAttenuation(), 20.0 / 34.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm95Attenuation(), 20.0 / 34.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm80Attenuation(), 20.0 / 33.0, FloatEpsilon);
        }
    }

    Y_UNIT_TEST(TestOneWordMultiHits) {
        TMovingWindow movingWindow(32, 1000);
        TWeightsHolder weights(Pool, 4, EStorageMode::Empty);
        weights.Add(0.5f);
        weights.Add(0.1f);
        weights.Add(0.1f);
        weights.Add(0.3f);
        movingWindow.Init(Pool, 4, weights);
        movingWindow.NewDoc();

        movingWindow.AddHit(1, 10);
        movingWindow.AddHit(1, 11);
        movingWindow.AddHit(1, 12);
        movingWindow.AddHit(1, 13);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcm(), 0.1, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcmAttenuation(), 0.1, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcmFullAttenuation(), 0.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm95Attenuation(), 0.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm80Attenuation(), 0.0, FloatEpsilon);

        movingWindow.AddHit(3, 2032);
        movingWindow.AddHit(3, 2033);
        movingWindow.AddHit(3, 2034);
        movingWindow.AddHit(3, 2035);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcm(), 0.3, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcmAttenuation(), 0.3 * 1000.0 / 3000.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcmFullAttenuation(), 0.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm95Attenuation(), 0.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm80Attenuation(), 0.0, FloatEpsilon);

        movingWindow.AddHit(0, 5029);
        movingWindow.AddHit(1, 5030);
        movingWindow.AddHit(2, 5031);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcm(), 0.7, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcmAttenuation(), 0.7 * 1000.0 / 5999.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcmFullAttenuation(), 0.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm95Attenuation(), 0.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm80Attenuation(), 0.0, FloatEpsilon);

        movingWindow.AddHit(3, 5032);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcm(), 1.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcMaxWcmAttenuation(), 1000.0 / 6000.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcmFullAttenuation(), 1000.0 / 6000.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm95Attenuation(), 1000.0 / 6000.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(movingWindow.WindowAcc.CalcWcm80Attenuation(), 1000.0 / 6000.0, FloatEpsilon);
    }

    Y_UNIT_TEST(TestMinWindow) {
        TMinWindow minWindow;
        minWindow.Init(Pool, 4);

        for (size_t counter = 0; counter < 3; ++counter) {
            minWindow.NewDoc();
            minWindow.AddHit(0, 0);
            minWindow.AddHit(1, 1);
            minWindow.AddHit(3, 2);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 0.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 0.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(200.0), 0.0, FloatEpsilon);

            minWindow.AddHit(2, 4);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 3.0 / 4.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 4.0 / 4.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(200.0), 1.0 - 4.0 / 200.0, FloatEpsilon);

            minWindow.AddHit(0, 5);
            minWindow.AddHit(1, 6);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 3.0 / 4.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 4.0 / 4.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(200.0), 1.0 - 4.0 / 200.0, FloatEpsilon);

            minWindow.AddHit(3, 7);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 1.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 4.0 / 4.0, FloatEpsilon);
            UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(200.0), 1.0 - 4.0 / 200.0, FloatEpsilon);
        }
    }

    Y_UNIT_TEST(TestMinWindow2) {
        TMinWindow minWindow;
        minWindow.Init(Pool, 5);
        minWindow.NewDoc();

        minWindow.AddHit(1, 5);
        minWindow.AddHit(4, 10);
        minWindow.AddHit(0, 15);
        minWindow.AddHit(2, 20);
        minWindow.AddHit(1, 400);
        minWindow.AddHit(4, 500);
        minWindow.AddHit(0, 600);
        minWindow.AddHit(2, 800);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 0.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 0.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(2000.0), 0.0, FloatEpsilon);

        minWindow.AddHit(3, 899);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 4.0 / 499.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 5.0 / 899.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(2000.0), 1.0 - 899.0 / 2000.0, FloatEpsilon);

        minWindow.AddHit(4, 900);
        minWindow.AddHit(0, 900);
        minWindow.AddHit(2, 900);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 4.0 / 499.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 5.0 / 899.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(2000.0), 1.0 - 899.0 / 2000.0, FloatEpsilon);

        minWindow.AddHit(1, 998);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 4.0 / 99.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 5.0 / 899.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(2000.0), 1.0 - 899.0 / 2000.0, FloatEpsilon);
    }

    Y_UNIT_TEST(TestMinWindow3) {
        TMinWindow minWindow;

        minWindow.Init(Pool, 2);
        minWindow.NewDoc();
        minWindow.AddHit(0, 3);
        minWindow.AddHit(1, 4);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 1.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 2.0 / 4.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(10.0), 1.0 - 0.4, FloatEpsilon);

        minWindow.Init(Pool, 1);
        minWindow.NewDoc();
        minWindow.AddHit(0, 5);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 1.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 1.0 / 5.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(10.0), 1.0 - 0.5, FloatEpsilon);

        minWindow.Init(Pool, 3);
        minWindow.NewDoc();
        minWindow.AddHit(0, 3);
        minWindow.AddHit(1, 6);
        minWindow.AddHit(2, 9);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcMinWindowSize(), 2.0 / 6.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageMinOffset(), 3.0 / 9.0, FloatEpsilon);
        UNIT_ASSERT_DOUBLES_EQUAL(minWindow.CalcFullMatchCoverageStartingPercent(10.0), 1.0 - 0.9, FloatEpsilon);
    }
};
