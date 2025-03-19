#include <kernel/factor_storage/factor_view.h>
#include <kernel/factor_storage/factor_storage.h>

#include <search/begemot/rules/query_factors/proto/query_factors.pb.h>
#include <kernel/web_factors_info/factors_gen.h>

#include <kernel/fill_factors_codegen/ut/test_fill.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TCheck) {
    Y_UNIT_TEST(TSimpleTest) {
        TFactorDomain::SetGlobalUniverse(EFactorUniverse::WEB);
        TSlicesMetaInfo metaInfo;
        NFactorSlices::EnableSlices(metaInfo, EFactorSlice::WEB_PRODUCTION);
        metaInfo.SetNumFactors(EFactorSlice::WEB_PRODUCTION, FI_FACTOR_COUNT);
        TFactorStorage storage(metaInfo);
        auto view = storage.CreateViewFor(EFactorSlice::WEB_PRODUCTION);
        NTest::TestMethod(NBg::NProto::TQueryFactors{}, view);
    }
}
