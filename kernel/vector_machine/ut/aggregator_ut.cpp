#include <kernel/vector_machine/aggregators.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

using namespace NVectorMachine;

class TAggregatorTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TAggregatorTest);
        UNIT_TEST(TestAvgTopScore);
        UNIT_TEST(TestAvgTopScoreWeighted);
        UNIT_TEST(TestMinTopScore);
        UNIT_TEST(TestMinTopScoreWeighted);
        UNIT_TEST(TestAvgTopWeight);
        UNIT_TEST(TestMinTopWeight);
        UNIT_TEST(TestIdentity);
        UNIT_TEST(TestAvgTopScoreXWeight);
    UNIT_TEST_SUITE_END();

public:
    void TestAvgTopScore() {
        TAvgTopScoreAggregator aggregator({0.f, 2.f, 0.5f, 1.f});

        TVector<float> scores = {1.f, 1.f, 2.f, 2.f};
        TVector<float> weights = {0.5f, 1.f, 0.5f, 1.f};
        TVector<float> expected = {2.f, 2.f, 2.f, 1.5};

        TVector<float> features(expected.size());
        aggregator.CalcFeatures(scores, weights, TArrayRef<float>(features));

        for (size_t i = 0; i < expected.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(features[i], expected[i], 1e-5);
        }
    }

    void TestAvgTopScoreWeighted() {
        TAvgTopScoreWeightedAggregator aggregator({0.f, 2.f, 0.5f, 1.f});
        TVector<float> scores = {1.f, 1.f, 2.f, 2.f};
        TVector<float> weights = {0.5f, 1.f, 0.5f, 1.f};
        TVector<float> expected = {2.f, 1.5f, 1.5f, 1.125f};

        TVector<float> features(expected.size());
        aggregator.CalcFeatures(scores, weights, TArrayRef<float>(features.data(), features.size()));

        UNIT_ASSERT_VALUES_EQUAL(features.size(), expected.size());

        for (size_t i = 0; i < expected.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(features[i], expected[i], 1e-5);
        }
    }

    void TestMinTopScore() {
        TMinTopScoreAggregator aggregator({0.f, 2.f, 0.5f, 1.f});

        TVector<float> scores = {1.f, 1.f, 2.f, 2.f};
        TVector<float> weights = {0.5f, 1.f, 0.5f, 1.f};
        TVector<float> expected = {2.f, 2.f, 2.f, 1.f};

        TVector<float> features(expected.size());
        aggregator.CalcFeatures(scores, weights, TArrayRef<float>(features.data(), features.size()));

        UNIT_ASSERT_VALUES_EQUAL(features.size(), expected.size());

        for (size_t i = 0; i < expected.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(features[i], expected[i], 1e-5);
        }
    }

    void TestMinTopScoreWeighted() {
        TMinTopScoreWeightedAggregator aggregator({0.f, 2.f, 0.5f, 1.f});

        TVector<float> scores = {1.f, 1.f, 2.f, 2.f};
        TVector<float> weights = {0.5f, 1.f, 0.5f, 1.f};
        TVector<float> expected = {2.f, 1.f, 1.f, 0.5f};

        TVector<float> features(expected.size());
        aggregator.CalcFeatures(scores, weights, TArrayRef<float>(features.data(), features.size()));

        UNIT_ASSERT_VALUES_EQUAL(features.size(), expected.size());

        for (size_t i = 0; i < expected.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(features[i], expected[i], 1e-5);
        }
    }

    void TestAvgTopWeight() {
        TAvgTopWeightAggregator aggregator({0.f, 2.f, 0.5f, 1.f});

        TVector<float> scores = {1.f, 1.f, 2.f, 2.f};
        TVector<float> weights = {0.5f, 1.f, 0.5f, 1.f};
        TVector<float> expected = {1.f, 1.f, 1.f, 0.75f};

        TVector<float> features(expected.size());
        aggregator.CalcFeatures(scores, weights, TArrayRef<float>(features.data(), features.size()));

        UNIT_ASSERT_VALUES_EQUAL(features.size(), expected.size());

        for (size_t i = 0; i < expected.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(features[i], expected[i], 1e-5);
        }
    }

    void TestMinTopWeight() {
        TMinTopWeightAggregator aggregator({0.f, 2.f, 0.5f, 1.f});

        TVector<float> scores = {1.f, 1.f, 2.f, 2.f};
        TVector<float> weights = {0.5f, 1.f, 0.5f, 1.f};
        TVector<float> expected = {1.f, 1.f, 1.f, 0.5f};

        TVector<float> features(expected.size());
        aggregator.CalcFeatures(scores, weights, TArrayRef<float>(features.data(), features.size()));

        UNIT_ASSERT_VALUES_EQUAL(features.size(), expected.size());

        for (size_t i = 0; i < expected.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(features[i], expected[i], 1e-5);
        }
    }

    void TestIdentity() {
        TIdentityAggregator aggregator({1, 2, 3, 4});

        TVector<float> scores = {1.f, 1.f, 2.f, 2.f};
        TVector<float> weights = {0.5f, 1.f, 0.5f, 1.f};
        TVector<float> expected = {1.f, 1.f, 2.f, 2.f};

        TVector<float> features(expected.size());
        aggregator.CalcFeatures(scores, weights, TArrayRef<float>(features.data(), features.size()));

        UNIT_ASSERT_VALUES_EQUAL(features.size(), expected.size());

        for (size_t i = 0; i < expected.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(features[i], expected[i], 1e-5);
        }
    }

    void TestAvgTopScoreXWeight() {
        TAvgTopScoreXWeightAggregator aggregator({0.f, 0.1f, 1.f, 2.f, 3.f});

        TVector<float> scores = {1.f, 1.5f, 2.f, 2.5f};
        TVector<float> weights = {0.5f, 1.f, 0.5f, 1.f};
        TVector<float> expected = {2.5f, 2.5f, 1.83333f, 2.33333f, 2.f};

        TVector<float> features(expected.size());
        aggregator.CalcFeatures(scores, weights, TArrayRef<float>(features.data(), features.size()));

        UNIT_ASSERT_VALUES_EQUAL(features.size(), expected.size());

        for (size_t i = 0; i < expected.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(features[i], expected[i], 1e-5);
        }

    }
};

UNIT_TEST_SUITE_REGISTRATION(TAggregatorTest);
