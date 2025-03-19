#include "artifacts_registry.h"
#include <kernel/factor_slices/factor_slices.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/json/json_writer.h>

using namespace NFactorSlices;

void CheckArtDependenciesList(const IFactorsInfo * factors) {
    UNIT_ASSERT(factors);
    auto reg = NRelevArtifacts::GetWebSearchRegistry();

    THashSet<TString> arts;
    for(const NRelevArtifacts::TRelevArtifact& art: reg.GetRelevArtifact()) {
        arts.insert(art.GetName());
    }

     for (size_t i : xrange(factors->GetFactorCount())) {
        if (i != 1390) {
            continue;
        }

        NJson::TJsonValue extJson = factors->GetExtJson(i);
        if (!extJson.Has("UseArtifact")) {
            continue;
        }
        for(auto& val : extJson["UseArtifact"].GetArraySafe()) {
            TString name = val["Name"].GetStringSafe();
            Y_ENSURE(arts.contains(name), "unknown artifact: " << factors->GetFactorName(i) << " <- " << name );
         }
     }
}

Y_UNIT_TEST_SUITE(AllSlicesArtDepsCheck) {
    Y_UNIT_TEST(WebProduction) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_PRODUCTION)
        );
    }

    Y_UNIT_TEST(WebMeta) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_META)
        );
    }

    Y_UNIT_TEST(WebMetaPers) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_META_PERS)
        );
    }

    Y_UNIT_TEST(WebMetaItdItp) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_META_ITDITP)
        );
    }

    Y_UNIT_TEST(WebL1) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_L1)
        );
    }

    Y_UNIT_TEST(WebNewL1) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_NEW_L1)
        );
    }

    Y_UNIT_TEST(WebL2) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_L2)
        );
    }

    Y_UNIT_TEST(WebL3MxNet) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_L3_MXNET)
        );
    }

    Y_UNIT_TEST(WebFreshDetector) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_FRESH_DETECTOR)
        );
    }

    Y_UNIT_TEST(BegemotQueryFactors) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::BEGEMOT_QUERY_FACTORS)
        );
    }

    Y_UNIT_TEST(RapidClicks) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::RAPID_CLICKS)
        );
    }

    Y_UNIT_TEST(RapidPersClicks) {
        CheckArtDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::RAPID_PERS_CLICKS)
        );
    }
}
