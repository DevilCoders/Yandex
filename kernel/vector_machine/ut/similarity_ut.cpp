#include <kernel/vector_machine/transforms.h>
#include <kernel/vector_machine/similarities.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NVectorMachine;

class TSimilarityTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TSimilarityTest);
        UNIT_TEST(TestEmptyCos);
        UNIT_TEST(TestLinearSigmaDot);
        UNIT_TEST(TestDotNoSizeCheck);
    UNIT_TEST_SUITE_END();

public:
    void TestEmptyCos() {
        TEmbed left = {-10.f, 0.f, 10.f};
        TEmbed right = {0.f, 10.f, 10.f};
        float expected = 0.5;

        float feature = TCosSimilarity::CalcSim(left, right);
        UNIT_ASSERT_DOUBLES_EQUAL(feature, expected, 1e-5);
    }

    void TestLinearSigmaDot() {
        TEmbed left = {-10.f, 0.f, 10.f};
        TEmbed right = {0.f, 10.f, 10.f};
        float expected = 3;

        float feature = TDotSimilarity::CalcSim(left, right);
        feature = TSigmoidTransform().Transform(feature);
        feature = TLinearTransform(2, 1).Transform(feature);
        UNIT_ASSERT_DOUBLES_EQUAL(feature, expected, 1e-5);
    }

    void TestDotNoSizeCheck() {
        TEmbed left = {-10.f, 0.f, 10.f};
        TEmbed right = {0.f, 10.f, 10.f};
        const float expected = 100.f;

        UNIT_ASSERT_DOUBLES_EQUAL(TDotSimilarityNoSizeCheck::CalcSim(left, right), expected, 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(TDotSimilarityNoSizeCheck::CalcSim(left, TEmbed()), 0.f, 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(TDotSimilarityNoSizeCheck::CalcSim(TEmbed(), right), 0.f, 1e-5);
        UNIT_ASSERT_EXCEPTION(TDotSimilarityNoSizeCheck::CalcSim(TEmbed(10, 1.f), TEmbed(20, 1.f)), yexception);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TSimilarityTest);
