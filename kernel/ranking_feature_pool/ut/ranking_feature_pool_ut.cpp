#include <kernel/ranking_feature_pool/ranking_feature_pool.h>

#include <kernel/web_factors_info/factor_names.h>
#include <kernel/web_meta_factors_info/factor_names.h>
#include <kernel/factor_slices/factor_domain.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/xrange.h>

#include <google/protobuf/text_format.h>

using namespace NRankingFeaturePool;

Y_UNIT_TEST_SUITE(RankingFeaturePoolTest) {
    Y_UNIT_TEST(TestAbsent) {
        TFeatureInfo info;
        MakeAbsentFeatureInfo("web_production", info);
        UNIT_ASSERT_EQUAL(info.GetName(), "");
        UNIT_ASSERT_EQUAL(info.GetSlice(), "web_production");
    }

    Y_UNIT_TEST(TestWebInfo) {
        TFeatureInfo info;
        MakeFeatureInfo(*GetWebFactorsInfo(), 5, info);
        UNIT_ASSERT_EQUAL(info.GetSlice(), "web_production");
        UNIT_ASSERT_EQUAL(info.GetName(), "TRp2");
        UNIT_ASSERT(HasTag(info, "TG_DOC"));
        UNIT_ASSERT(!HasTag(info, "TG_USER"));
    }

    Y_UNIT_TEST(TestDomainToInfo) {
        NFactorSlices::TFactorDomain domain(EFactorSlice::WEB_PRODUCTION,
            EFactorSlice::WEB_META);
        TPoolInfo info;
        MakePoolInfo(domain, info);
        UNIT_ASSERT_EQUAL(info.FeatureInfoSize(), NSliceWebProduction::FI_FACTOR_COUNT + NWebMeta::NSliceWebMeta::FI_FACTOR_COUNT);
        {
            TStringStream borders;
            borders << "web_production[0;" << int(NSliceWebProduction::FI_FACTOR_COUNT) << ")"
                << " web_meta[" << int(NSliceWebProduction::FI_FACTOR_COUNT) << ";"
                << int(NSliceWebProduction::FI_FACTOR_COUNT) + int(NWebMeta::NSliceWebMeta::FI_FACTOR_COUNT) << ")";
            UNIT_ASSERT_EQUAL(GetBordersStr(info), borders.Str());
        }
        {
            const auto& fInfo = info.GetFeatureInfo(5);
            UNIT_ASSERT_EQUAL(fInfo.GetName(), "TRp2");
        }
    }

    Y_UNIT_TEST(TestToJson) {
        TFeatureInfo info;
        MakeFeatureInfo(*GetWebFactorsInfo(), 5, info);
        auto value = ToJson(info);
        UNIT_ASSERT_EQUAL(value["Name"], NJson::TJsonValue("TRp2"));
        UNIT_ASSERT(!value.Has("ExtJson"));
        value = ToJson(info, true);
        UNIT_ASSERT(value.Has("ExtJson"));
        UNIT_ASSERT_EQUAL(value["ExtJson"]["Index"], NJson::TJsonValue(5));
    }
}
