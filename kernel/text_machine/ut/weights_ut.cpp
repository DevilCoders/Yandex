#include <kernel/text_machine/parts/common/weights.h>
#include <kernel/text_machine/parts/common/types.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NTextMachine;
using namespace NTextMachine::NCore;

class TWeightsTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TWeightsTest);
        UNIT_TEST(TestEmpty);
        UNIT_TEST(TestUninitialized);
        UNIT_TEST(TestNorm);
        UNIT_TEST(TestChangeWeight);
    UNIT_TEST_SUITE_END();

private:
    TMemoryPool Pool{32 << 10};

public:
    void TestEmpty() {
        TWeightsHolder weights;
        weights.Init(Pool, 0, EStorageMode::Full);

        {
            TWeightsHelper helper(weights);
            helper.Finish();

            UNIT_ASSERT_EQUAL(weights.Count(), 0);
        }
    }

    void TestUninitialized() {
        TWeightsHolder weights;
        weights.Init(Pool, 1, EStorageMode::Full);

        {
            TWeightsHelper helper(weights);
            helper.Finish();

            UNIT_ASSERT_EQUAL(weights.Count(), 1);
            UNIT_ASSERT_DOUBLES_EQUAL(weights[0], 1.0f, 1e-4);
        }
    }

    void TestNorm() {
        {
            TWeightsHolder weights;
            weights.Init(Pool, 1, EStorageMode::Full);

            TWeightsHelper helper(weights);
            helper.SetWordWeight(0, 1000.0f);
            helper.Finish();

            UNIT_ASSERT_EQUAL(weights.Count(), 1);
            UNIT_ASSERT_DOUBLES_EQUAL(weights[0], 1.0f, 1e-4);
        }

        {
            TWeightsHolder weights;
            weights.Init(Pool, 2, EStorageMode::Full);

            TWeightsHelper helper(weights);
            helper.SetWordWeight(0, 1.0f);
            helper.SetWordWeight(1, 1.0f);
            helper.Finish();

            UNIT_ASSERT_EQUAL(weights.Count(), 2);
            UNIT_ASSERT_DOUBLES_EQUAL(weights[0], weights[1], 1e-4);
            UNIT_ASSERT_DOUBLES_EQUAL(weights[0] + weights[1], 1.0f, 1e-4);
        }

        {
            TWeightsHolder weights;
            weights.Init(Pool, 3, EStorageMode::Full);

            TWeightsHelper helper(weights);
            helper.SetWordWeight(0, 1.0f);
            helper.SetWordWeight(1, 2.0f);
            helper.SetWordWeight(2, 0.0f);
            helper.Finish();

            UNIT_ASSERT_EQUAL(weights.Count(), 3);
            UNIT_ASSERT_DOUBLES_EQUAL(2.0 * weights[0], weights[1], 1e-4);
            UNIT_ASSERT(weights[2] > 0.0f);
            UNIT_ASSERT_DOUBLES_EQUAL(weights[0] + weights[1] + weights[2], 1.0f, 1e-4);
        }
    }

    void TestChangeWeight() {
        {
            TWeightsHolder weights;
            weights.Init(Pool, 2, EStorageMode::Full);

            TWeightsHelper helper(weights);
            helper.SetWordWeight(0, 1.0f);
            helper.SetWordWeight(1, 0.0f);
            helper.SetWordWeight(1, 1.0f);
            helper.Finish();

            UNIT_ASSERT_EQUAL(weights.Count(), 2);
            UNIT_ASSERT_DOUBLES_EQUAL(weights[0], weights[1], 1e-4);
            UNIT_ASSERT_DOUBLES_EQUAL(weights[0] + weights[1], 1.0f, 1e-4);
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TWeightsTest);
