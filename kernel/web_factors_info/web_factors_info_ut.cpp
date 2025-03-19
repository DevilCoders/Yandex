#include "factor_names.h"

#include <kernel/factor_slices/factor_slices.h>
#include <kernel/web_factors_info/validators/impltime_ut_helper.h>
#include <kernel/web_factors_info/validators/dependency_ut_helper.h>
#include <search/lingboost/relev/features_saver.h>
#include <search/lingboost/production/web_production_features_calcer.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NFactorSlices;

class TWebFactorsInfoTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TWebFactorsInfoTest);
        UNIT_TEST(TestWebSlices);
        UNIT_TEST(TestL3ModelValue);
        UNIT_TEST(TestImplementationTimeFormat);
        UNIT_TEST(TestImplementationTimePresence);
        UNIT_TEST(TestDepsList);
        UNIT_TEST(TestTextMachineTag);
    UNIT_TEST_SUITE_END();

public:
    void TestWebSlices() {
        const auto& metaInfo = TGlobalSlicesMetaInfo::Instance();

        Cdbg << "WEB_PRODUCTION: " << metaInfo.GetNumFactors(EFactorSlice::WEB_PRODUCTION) << Endl;

        UNIT_ASSERT(metaInfo.IsSliceInitialized(EFactorSlice::WEB_PRODUCTION));
        UNIT_ASSERT(metaInfo.GetNumFactors(EFactorSlice::WEB_PRODUCTION) > 0);
        UNIT_ASSERT(metaInfo.GetFactorsInfo(EFactorSlice::WEB_PRODUCTION));
    }

    void TestL3ModelValue() {
        const IFactorsInfo* info = GetWebFactorsInfo();
        UNIT_ASSERT(info->GetL3ModelValueIndex() == FI_MATRIXNET);
    }

    void TestImplementationTimeFormat() {
        EnsureImplementationTimeFormat(GetWebFactorsInfo());
    }

    void TestImplementationTimePresence() {
        EnsureImplementationTimePresence(GetWebFactorsInfo(), 1615);
    }

    void TestDepsList() {
        NFactorsInfoValidators::CheckDependenciesList(GetWebFactorsInfo());
    }

    void TestTextMachineTag() {
        TVector<NLingBoost::TFeatureIdsTable> table;
        NLingBoost::GetAllWebProductionSliceFeaturesOnL3(table);

        const IFactorsInfo* info = NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_PRODUCTION);
        Y_ENSURE(info);
        for(auto& subTable : table) {
            for(auto& featureAndId : subTable) {
                const size_t featureId = featureAndId.Index;
                Y_ENSURE(info->HasTagName(featureId, "TG_TEXT_MACHINE"),
                    "TG_TEXT_MACHINE is absent in " << info->GetFactorName(featureId)
                );
            }
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TWebFactorsInfoTest);
