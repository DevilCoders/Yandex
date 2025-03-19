#include <library/cpp/testing/unittest/registar.h>
#include <kernel/nn/tf_apply/inputs.h>

Y_UNIT_TEST_SUITE(UtilsTestSuite){
    Y_UNIT_TEST(WrapInVectorMoveTest) {
        TVector<float> sample(10, 0.);
        TVector<TVector<float>> expanded = NTFModel::WrapInVector(std::move(sample));

        UNIT_ASSERT(expanded.size() == 1);
        UNIT_ASSERT(expanded[0].size() == 10);

        TVector<int> expandedInt = NTFModel::WrapInVector(10);
        UNIT_ASSERT(expandedInt.size() == 1);
        UNIT_ASSERT(expandedInt[0] == 10);
    }

    Y_UNIT_TEST(WrapInVectorTest) {
        TVector<float> sample(10, 0.);
        TVector<TVector<float>> expanded = NTFModel::WrapInVector(sample);

        UNIT_ASSERT(expanded.size() == 1);
        UNIT_ASSERT(expanded[0].size() == 10);

        TVector<int> expandedInt = NTFModel::WrapInVector(10);
        UNIT_ASSERT(expandedInt.size() == 1);
        UNIT_ASSERT(expandedInt[0] == 10);
    }
};
