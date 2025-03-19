#include "bundle.h"

#include <library/cpp/testing/unittest/registar.h>
#include <kernel/matrixnet/mn_dynamic.h>

#include <util/stream/str.h>

Y_UNIT_TEST_SUITE(NMatrixnetTBundleTest) {

Y_UNIT_TEST(SimpleTest) {
    const auto mn1 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn2 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const NMatrixnet::TRankModelVector matrixnets =
        {NMatrixnet::TWeightedMatrixnet(mn1, {1.0, 0.0}),
        NMatrixnet::TWeightedMatrixnet(mn2, {2.0, 0.0})};

    const auto modelInfo = NMatrixnet::TModelInfo();
    const NMatrixnet::TBundle bundle(matrixnets, modelInfo);

    UNIT_ASSERT_EQUAL(matrixnets.size(), bundle.Matrixnets.size());
    for (size_t i = 0; i < matrixnets.size(); ++i) {
        UNIT_ASSERT_EQUAL(matrixnets[i].Matrixnet.Get(), bundle.Matrixnets[i].Matrixnet.Get());
        UNIT_ASSERT_EQUAL(matrixnets[i].Renorm, bundle.Matrixnets[i].Renorm);
    }
}

Y_UNIT_TEST(TestGetInfo) {
    const auto mn1 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const auto mn2 = MakeAtomicShared<NMatrixnet::TMnSseDynamic>();
    const NMatrixnet::TRankModelVector matrixnets =
        {NMatrixnet::TWeightedMatrixnet(mn1, {1.0, 4.0}),
        NMatrixnet::TWeightedMatrixnet(mn2, {2.0, 0.0})};

    const auto modelInfo = NMatrixnet::TModelInfo();
    const NMatrixnet::TBundle bundle(matrixnets, modelInfo);

    TStringStream s;
    for (auto&& [key, value] : *bundle.GetInfo()) {
        s << key << ' ' << value << ' ';
    }
    UNIT_ASSERT_VALUES_EQUAL(s.Str(), "~0_bias 4 ~0_scale 1 ~1_bias 0 ~1_scale 2 ");
}

Y_UNIT_TEST(TestRenorm) {
    NMatrixnet::TBundleRenorm rnm0;
    UNIT_ASSERT_VALUES_EQUAL(rnm0.Apply(888), 888);

    NMatrixnet::TBundleRenorm rnm1{30, 42};
    UNIT_ASSERT_EQUAL(rnm1.Apply(10), 342);
    NMatrixnet::TBundleRenorm rnm2{10, 4};

    UNIT_ASSERT_VALUES_EQUAL(NMatrixnet::TBundleRenorm::Sum(rnm1, rnm2), NMatrixnet::TBundleRenorm(40, 46));
    UNIT_ASSERT_VALUES_EQUAL(NMatrixnet::TBundleRenorm::Compose(rnm1, rnm2), NMatrixnet::TBundleRenorm(300, 162));
}

};
