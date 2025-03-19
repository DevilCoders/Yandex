#include <kernel/click_sim/word_vector.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TWordVectorTest) {
    Y_UNIT_TEST(TestStripByMinWeight) {
        THashMap<TString, double> weights;
        weights["a"] = 0.1;
        weights["b"] = 0.2;
        weights["c"] = 0.8;
        weights["d"] = 0.06;
        weights["e"] = 0.5;

        NClickSim::TWordVector vector(std::move(weights));
        vector.StripByMinWeight(0.11);
        UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().at("b"), 0.2);
        UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().at("c"), 0.8);
        UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().at("e"), 0.5);
        UNIT_ASSERT(!vector.GetWeights().contains("a"));
        UNIT_ASSERT(!vector.GetWeights().contains("d"));
    }

    Y_UNIT_TEST(StripByMinL2Share) {
            THashMap<TString, double> weights;
            weights["a"] = 0.1;
            weights["b"] = 0.2;
            weights["c"] = 0.2;

            NClickSim::TWordVector vector(std::move(weights));
            vector.StripByMinL2Share(0.5);
            UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().at("b"), 0.2);
            UNIT_ASSERT(!vector.GetWeights().contains("a"));
            UNIT_ASSERT(!vector.GetWeights().contains("c"));

            weights["a"] = 0.1;
            weights["b"] = 0.2;
            weights["c"] = 0.2;
            vector = NClickSim::TWordVector(std::move(weights));

            vector.StripByMinL2Share(1.0);
            UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().size(), 3);

            vector.StripByMinL2Share(0.94);
            UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().at("b"), 0.2);
            UNIT_ASSERT_VALUES_EQUAL(vector.GetWeights().at("c"), 0.2);
            UNIT_ASSERT(!vector.GetWeights().contains("a"));
    }
}
