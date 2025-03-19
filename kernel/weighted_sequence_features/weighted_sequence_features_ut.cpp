#include "weighted_sequence_features.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/string.h>

Y_UNIT_TEST_SUITE(WeightedSequenceFeatures) {
    struct TSimpleWidged :
        public NWeighedSequenceFeatures::TWidget
        <
        NWeighedSequenceFeatures::TUnitMaxWeihgt,
        NWeighedSequenceFeatures::TUnitMaxFeature,
        NWeighedSequenceFeatures::TUnitMaxWeightedFeature
        >
    {};

    std::pair<float, float> Calcer(
        TArrayRef<float> weights,
        TArrayRef<float> features) {
        TSimpleWidged widget; //no any allocations!
        widget.PrepareState(weights, features);

        return {
            NWeighedSequenceFeatures::CalcMaxWFNormedMaxW(widget),
            NWeighedSequenceFeatures::CalcMaxWF(widget)
        };
    }

    Y_UNIT_TEST(ReadmeExample) {
        auto res = Calcer(
            TVector<float>{0.5f, 0.7f},
            TVector<float>{0.9f, 0.6f}
        );

        UNIT_ASSERT_VALUES_EQUAL(res.second, Max(0.5f * 0.9f, 0.7f * 0.6f));
        UNIT_ASSERT_VALUES_EQUAL(res.first, (Max(0.5f * 0.9f, 0.7f * 0.6f)) / (0.7f) );
    }


    struct TAllUnits :
        public NWeighedSequenceFeatures::TWidget
        <
        NWeighedSequenceFeatures::TUnitMaxWeihgt,
        NWeighedSequenceFeatures::TUnitMaxFeature,
        NWeighedSequenceFeatures::TUnitMaxWeightedFeature,
        NWeighedSequenceFeatures::TUnitMinWeightedFeature,
        NWeighedSequenceFeatures::TUnitFeaturesValuesSum,
        NWeighedSequenceFeatures::TUnitWeightedFeaturesSum,
        NWeighedSequenceFeatures::TUnitWeightsValuesSum,
        NWeighedSequenceFeatures::TUnitCount
        >
    {};

    Y_UNIT_TEST(AllUnits) {
        TAllUnits widget;
        widget.PrepareState(
            TVector<float>{0.5f, 0.7f},
            TVector<float>{0.9f, 0.6f}
        );

        UNIT_ASSERT_VALUES_EQUAL(
            CalcMaxWF(widget), Max(0.5f * 0.9f, 0.7f * 0.6f)
        );
        UNIT_ASSERT_VALUES_EQUAL(
            CalcMaxWFNormedMaxW(widget), (Max(0.5f * 0.9f, 0.7f * 0.6f)) / (0.7f)
        );

        UNIT_ASSERT_VALUES_EQUAL(
            CalcSumFNormedCount(widget), (0.9f + 0.6f) / 2
        );

        UNIT_ASSERT_VALUES_EQUAL(
            CalcMinWFNormedMaxW(widget), Min(0.5f * 0.9f, 0.7f * 0.6f) / 0.7f
        );

        UNIT_ASSERT_VALUES_EQUAL(
            CalcSumWFNormedSumW(widget), (0.5f * 0.9f + 0.7f * 0.6f) / (0.5f + 0.7f)
        );
    }


    Y_UNIT_TEST(TTopCheck) {
        TAllUnits widget;
        NWeighedSequenceFeatures::ApplyThroughTopFeaturesFilter<2>(
            widget,
            TVector<float>{0.1f, 0.2f, 0.3f},
            TVector<float>{0.1f, 0.2f, 0.3f}
        );

        UNIT_ASSERT_VALUES_EQUAL(
            CalcMaxWF(widget), 0.3f * 0.3f
        );
        UNIT_ASSERT_VALUES_EQUAL(
            CalcSumFNormedCount(widget), (0.3f + 0.2f) / 2
        );
    }
}

