#include "multifeature_softmax.h"
#include <library/cpp/testing/unittest/registar.h>

using namespace NExtendedMx;
using namespace NExtendedMx::NMultiFeatureSoftmax;

class TMultiFeatureSoftmaxTest : public NUnitTest::TTestBase {
    UNIT_TEST_SUITE(TMultiFeatureSoftmaxTest);
        UNIT_TEST(TestFillSubtargetFactors);
    UNIT_TEST_SUITE_END();

    void AssertFactors(const NExtendedMx::TFactors& factors, const NExtendedMx::TFactors& etalon, size_t factorCount) {
        UNIT_ASSERT(factors.size() == etalon.size());
        for (size_t i = 0; i < factors.size(); ++i) {
            UNIT_ASSERT(factors[i].size() == factorCount);
            UNIT_ASSERT(factors[i].size() == etalon[i].size());
            for (size_t j = 0; j < factors[i].size(); ++j) {
                UNIT_ASSERT_DOUBLES_EQUAL_C(factors[i][j], etalon[i][j], 1e-12, TStringBuilder() << " at (" << i << "," << j << ")");
            }
        }
    }

    TMultiFeatureParams InitParams(const TVector<size_t>& positions, const TVector<TString>& viewTypes) {
        TMultiFeatureParams mfp;
        mfp.Features.push_back({"Pos", TSafeVector<NSc::TValue>{}});
        mfp.Features.push_back({"ViewType", TSafeVector<NSc::TValue>()});
        auto& posFeat = mfp.Features[0];
        auto& vtFeat = mfp.Features[1];
        for (size_t i = 0; i < positions.size(); ++i) {
            posFeat.Values.push_back(NSc::TValue(positions[i]));
            for (size_t j = 0; j < viewTypes.size(); ++j) {
                mfp.Combinations.push_back(TCombination{});
                mfp.Combinations.back().push_back(i);
                mfp.Combinations.back().push_back(j);
                if (i == 0) {
                    vtFeat.Values.push_back(NSc::TValue(viewTypes[j]));
                }
            }
        }
        mfp.NoPosition.push_back(NSc::TValue(100));
        mfp.NoPosition.push_back(NSc::TValue(""));
        return mfp;
    }


public:
    void TestFillSubtargetFactors() {
        static const TVector<float> blenderFactors = {0.9f, 1.5f, 0.3f};
        TMultiFeatureParams mfp = InitParams({0, 5, 10}, {"high", "small"});
        mfp.FakeFeaturesMode = FFM_AS_INTEGER;
        mfp.Validate();

        NExtendedMx::TFactors factors;
        FillSubtargetFactors(factors, &blenderFactors[0], blenderFactors.size(), mfp);
        AssertFactors(
            factors,
            {{ 0, 0, 0.9f, 1.5f, 0.3f},
             { 0, 1, 0.9f, 1.5f, 0.3f},
             { 5, 0, 0.9f, 1.5f, 0.3f},
             { 5, 1, 0.9f, 1.5f, 0.3f},
             {10, 0, 0.9f, 1.5f, 0.3f},
             {10, 1, 0.9f, 1.5f, 0.3f},},
            mfp.GetFakeFeaturesCount() + blenderFactors.size()
        );

        factors.clear();
        mfp.FakeFeaturesMode = FFM_AS_BINARY;
        mfp.Validate();
        FillSubtargetFactors(factors, &blenderFactors[0], blenderFactors.size(), mfp);
        AssertFactors(
            factors,
            {{1, 0, 0, 1, 0, 0.9f, 1.5f, 0.3f},
             {1, 0, 0, 0, 1, 0.9f, 1.5f, 0.3f},
             {0, 1, 0, 1, 0, 0.9f, 1.5f, 0.3f},
             {0, 1, 0, 0, 1, 0.9f, 1.5f, 0.3f},
             {0, 0, 1, 1, 0, 0.9f, 1.5f, 0.3f},
             {0, 0, 1, 0, 1, 0.9f, 1.5f, 0.3f}},
            mfp.GetFakeFeaturesCount() + blenderFactors.size()
        );

        factors.clear();
        mfp.FakeFeaturesMode = FFM_AS_BINARY | FFM_AS_INTEGER;
        mfp.Validate();
        FillSubtargetFactors(factors, &blenderFactors[0], blenderFactors.size(), mfp);
        AssertFactors(
            factors,
            {{ 0, 1, 0, 0, 0, 1, 0, 0.9f, 1.5f, 0.3f},
             { 0, 1, 0, 0, 1, 0, 1, 0.9f, 1.5f, 0.3f},
             { 5, 0, 1, 0, 0, 1, 0, 0.9f, 1.5f, 0.3f},
             { 5, 0, 1, 0, 1, 0, 1, 0.9f, 1.5f, 0.3f},
             {10, 0, 0, 1, 0, 1, 0, 0.9f, 1.5f, 0.3f},
             {10, 0, 0, 1, 1, 0, 1, 0.9f, 1.5f, 0.3f}},
            mfp.GetFakeFeaturesCount() + blenderFactors.size()
        );
    }
};

UNIT_TEST_SUITE_REGISTRATION(TMultiFeatureSoftmaxTest);
