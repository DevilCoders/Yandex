#include "factor_names.h"
#include <kernel/web_factors_info/validators/impltime_ut_helper.h>
#include <kernel/generated_factors_info/simple_factors_info.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NFactorSlices;

class TBegemotQueryFactorsInfoTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TBegemotQueryFactorsInfoTest);
        UNIT_TEST(TestImplementationTimeFormat);
        UNIT_TEST(TestImplementationTimePresence);
    UNIT_TEST_SUITE_END();

public:
    void TestImplementationTimeFormat() {
        TWebFactorsInfo<NBegemotQueryFactors::TFactorInfo> info(NSliceBegemotQueryFactors::FI_FACTOR_COUNT, NBegemotQueryFactors::GetFactorsInfo());
        EnsureImplementationTimeFormat(&info);
    }

    void TestImplementationTimePresence() {
        TWebFactorsInfo<NBegemotQueryFactors::TFactorInfo> info(NSliceBegemotQueryFactors::FI_FACTOR_COUNT, NBegemotQueryFactors::GetFactorsInfo());
        EnsureImplementationTimePresence(&info, 179);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TBegemotQueryFactorsInfoTest);
