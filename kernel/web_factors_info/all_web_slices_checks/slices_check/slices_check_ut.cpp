#include <kernel/factor_slices/factor_slices.h>
#include <kernel/web_factors_info/validators/dependency_ut_helper.h>
#include <kernel/web_factors_info/validators/borders_ut_helper.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NFactorSlices;

Y_UNIT_TEST_SUITE(AllSlicesDepsCheck) {
    Y_UNIT_TEST(WebProduction) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_PRODUCTION)
        );
    }

    Y_UNIT_TEST(WebMeta) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_META)
        );
    }

    Y_UNIT_TEST(WebMetaPers) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_META_PERS)
        );
    }

    Y_UNIT_TEST(WebMetaItdItp) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_META_ITDITP)
        );
    }

    Y_UNIT_TEST(WebL1) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_L1)
        );
    }

    Y_UNIT_TEST(WebNewL1) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_NEW_L1)
        );
    }

    Y_UNIT_TEST(WebL2) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_L2)
        );
    }

    Y_UNIT_TEST(WebL3MxNet) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_L3_MXNET)
        );
    }

    Y_UNIT_TEST(WebFreshDetector) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_FRESH_DETECTOR)
        );
    }

    Y_UNIT_TEST(WebItdItp) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_ITDITP)
        );
    }

    Y_UNIT_TEST(ItdItpUserHistory) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::ITDITP_USER_HISTORY)
        );
    }

    Y_UNIT_TEST(BegemotQueryFactors) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::BEGEMOT_QUERY_FACTORS)
        );
    }

    Y_UNIT_TEST(Personalization) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::PERSONALIZATION)
        );
    }

    Y_UNIT_TEST(RapidClicks) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::RAPID_CLICKS)
        );
    }

    Y_UNIT_TEST(RapidPersClicks) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::RAPID_PERS_CLICKS)
        );
    }

    Y_UNIT_TEST(WebProductionFormulaFeatures) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_PRODUCTION_FORMULA_FEATURES)
        );
    }

    Y_UNIT_TEST(RapidClicksL2) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::RAPID_CLICKS_L2)
        );
    }

    Y_UNIT_TEST(WebRtModels) {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_RTMODELS)
        );
    }
}

namespace {
    static constexpr std::array WebFactorSlices {
        EFactorSlice::WEB_PRODUCTION,
        EFactorSlice::WEB_META,
        EFactorSlice::WEB_META_PERS,
        EFactorSlice::WEB_META_ITDITP,
        EFactorSlice::WEB_L1,
        EFactorSlice::WEB_NEW_L1,
        EFactorSlice::WEB_L2,
        EFactorSlice::WEB_L3_MXNET,
        EFactorSlice::WEB_FRESH_DETECTOR,
        EFactorSlice::WEB_ITDITP,
        EFactorSlice::ITDITP_USER_HISTORY,
        EFactorSlice::BEGEMOT_QUERY_FACTORS,
        EFactorSlice::PERSONALIZATION,
        EFactorSlice::RAPID_CLICKS,
        EFactorSlice::RAPID_PERS_CLICKS,
        EFactorSlice::WEB_PRODUCTION_FORMULA_FEATURES,
        EFactorSlice::RAPID_CLICKS_L2,
        EFactorSlice::WEB_RTMODELS,
    };
}

Y_UNIT_TEST_SUITE(TestBorders) {
    Y_UNIT_TEST(TestBorders) {
        for (auto slice : WebFactorSlices) {
            NFactorsInfoValidators::CheckBorders(
                NFactorSlices::GetSlicesInfo()->GetFactorsInfo(slice)
            );
        }
    }
}
