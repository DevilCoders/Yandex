#include "factor_names.h"
#include <kernel/web_factors_info/validators/impltime_ut_helper.h>
#include <kernel/web_factors_info/validators/dependency_ut_helper.h>
#include <kernel/generated_factors_info/simple_factors_info.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NFactorSlices;

class TWebMetaFactorsInfoTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TWebMetaFactorsInfoTest);
        UNIT_TEST(TestImplementationTimeFormat);
        UNIT_TEST(TestImplementationTimePresence);
    UNIT_TEST_SUITE_END();

public:
    void TestImplementationTimeFormat() {
        TWebFactorsInfo<NWebMeta::TFactorInfo> info(NSliceWebMeta::FI_FACTOR_COUNT, NWebMeta::GetFactorsInfo());
        EnsureImplementationTimeFormat(&info);
    }

    void TestImplementationTimePresence() {
        TWebFactorsInfo<NWebMeta::TFactorInfo> info(NSliceWebMeta::FI_FACTOR_COUNT, NWebMeta::GetFactorsInfo());
        EnsureImplementationTimePresence(&info, 276);
    }

    void TestDepsList() {
        NFactorsInfoValidators::CheckDependenciesList(
            NFactorSlices::GetSlicesInfo()->GetFactorsInfo(EFactorSlice::WEB_META)
        );
    }
};

UNIT_TEST_SUITE_REGISTRATION(TWebMetaFactorsInfoTest);
