#include <library/cpp/testing/unittest/registar.h>
#include <kernel/prs_log/serializer/serialize.h>
#include <kernel/web_factors_info/factors_gen.h>
#include <kernel/web_meta_factors_info/factors_gen.h>

#include <util/random/random.h>
#include <util/generic/ymath.h>

using namespace NPrsLog;

namespace {
    const size_t TEST_SLICE_SIZE = 2000;

    bool NoisyEquals(const float f1, const float f2, const float noise) {
        return Abs(f1 - f2) <= noise;
    }

    TVector<float> GenerateRandomVector(const size_t size) {
        TVector<float> result(Reserve(size));
        for (size_t i = 0; i < size; ++i) {
            result.push_back(RandomNumber<float>());
        }
        return result;
    }

    TWebDocument GenerateRandomDocInfo(const TString& url, const TVector<float>& constSlice = {}) {
        TWebDocument result;
        result.Url = url;
        result.DocId = ToString(RandomNumber<ui64>());
        result.L2Relevance = RandomNumber<float>();

        result.WebFeatures = GenerateRandomVector(TEST_SLICE_SIZE);
        result.WebMetaFeatures = constSlice;
        return result;
    }

    bool SlicesEqual(const TVector<float>& first, const TVector<float>& second, const float eps) {
        if (first.size() != second.size()) {
            return false;
        }
        for (size_t i = 0; i < first.size(); ++i) {
            if (!NoisyEquals(first[i], second[i], eps)) {
                return false;
            }
        }
        return true;
    }

    bool WebDocumentsEqual(const TWebDocument& first, const TWebDocument& second, const float eps) {
        if (first.DocId != second.DocId) {
            return false;
        }
        if (first.Url != second.Url) {
            return false;
        }
        if (!NoisyEquals(first.L2Relevance, second.L2Relevance, eps)) {
            Cerr << "L2Relevance not equal: " << first.L2Relevance << " != " << second.L2Relevance << Endl;
            return false;
        }
        if (!SlicesEqual(first.WebFeatures, second.WebFeatures, eps)) {
            return false;
        }
        if (!SlicesEqual(first.WebMetaFeatures, second.WebMetaFeatures, eps)) {
            return false;
        }
        return true;
    }

    bool WebDataEqual(const TWebData& first, const TWebData& second, const float eps) {
        if (first.Documents.size() != second.Documents.size()) {
            return false;
        }
        for (size_t i = 0; i < first.Documents.size(); ++i) {
            if (!WebDocumentsEqual(first.Documents[i], second.Documents[i], eps)) {
                return false;
            }
        }
        if (first.WebFeaturesIds.size() != second.WebFeaturesIds.size()) {
            return false;
        }
        for (size_t i = 0; i < first.WebFeaturesIds.size(); ++i) {
            if (first.WebFeaturesIds[i] != second.WebFeaturesIds[i]) {
                return false;
            }
        }
        if (first.WebMetaFeaturesIds.size() != second.WebMetaFeaturesIds.size()) {
            return false;
        }
        for (size_t i = 0; i < first.WebMetaFeaturesIds.size(); ++i) {
            if (first.WebMetaFeaturesIds[i] != second.WebMetaFeaturesIds[i]) {
                return false;
            }
        }
        return true;
    }
};


Y_UNIT_TEST_SUITE(TestSerializerSuite) {
    const TVector<ESerializerType> TYPES = {ESerializerType::UniformBound, ESerializerType::FactorStorage};

    Y_UNIT_TEST(EmptyTest) {
        TWebData webData;
        for (const auto serializer : TYPES) {
            TString serialized = SerializeToBase64(webData, serializer);
            TWebData deserialized = DeserializeFromBase64(serialized);
            UNIT_ASSERT(WebDataEqual(webData, deserialized, 0.005));
        }
    }

    Y_UNIT_TEST(PackUnpackTest) {
        TVector<float> constSlice = GenerateRandomVector(10);

        TWebData webData;
        webData.Documents = {
            GenerateRandomDocInfo("https://vk.com/feed", constSlice),
            GenerateRandomDocInfo("https://music.yandex.ru/radio", constSlice),
            GenerateRandomDocInfo("https://www.google.com/maps/@53.718879,27.986709,7z?hl=en", constSlice),
            GenerateRandomDocInfo("https://charybary.ru/sushhnosti/kak-stat-feey-vinks/", constSlice),
            GenerateRandomDocInfo("https://ru.wikipedia.org/wiki/%D0%98%D0%BF%D0%BE%D1%82%D0%B5%D0%BA%D0%B0", constSlice),
            GenerateRandomDocInfo("https://yandex.by/search/?text=wiki%20путин&clid=1955453&win=335&rdrnd=420557&lr=157&redircnt=1545820363.1", constSlice),
            GenerateRandomDocInfo("https://www.youtube.com/watch?v=89lZ_xWSQjw", constSlice)
        };

        webData.WebFeaturesIds = {
            NSliceWebProduction::FI_BCLM,
            NSliceWebProduction::FI_VISITORS_RETURN_MONTH_NUMBER,
            NSliceWebProduction::FI_TITLE_BM15_K01,
            NSliceWebProduction::FI_TEXT_COSINE_MATCH_MAX_PREDICTION,
            NSliceWebProduction::FI_FIELD_SET1_BM15_FLOG_K0001,
            NSliceWebProduction::FI_FIELD_SET3_BCLM_WEIGHTED_FLOG_W0_K0001,
            NSliceWebProduction::FI_BODY_CHAIN0_WCM,
            NSliceWebProduction::FI_BODY_MIN_WINDOW_SIZE,
            NSliceWebProduction::FI_DOUBLE_FRC_ANNOTATION_MAX_VALUE_WEIGHTED,
            NSliceWebProduction::FI_LONG_CLICK_SP_COSINE_MATCH_MAX_PREDICTION,
            NSliceWebProduction::FI_QUERY_DWELL_TIME_MIX_MATCH_WEIGHTED_VALUE,
            NSliceWebProduction::FI_US_LONG_PERIOD_URL_DT180_AVG,
            NSliceWebProduction::FI_UB_LONG_PERIOD_URL_DT_URL_H_CHILDREN_CUT_600,
            NSliceWebProduction::FI_QFUF_ALL_SUM_WF_SUM_W_FIELD_SET3_BCLM_WEIGHTED_FLOG_W0_K0_001,
            NSliceWebProduction::FI_QFUF_ALL_MAX_F_TEXT_BOCM11_NORM256,
            NSliceWebProduction::FI_RANDOMLOGHOST_UB_LONG_PERIOD_URL_DT_URL_H_CHILDREN_CUT_600_REG_PERCENTALE_90,
            NSliceWebProduction::FI_DSSM_BOOSTING_XF_ONE_SE_AM_SS_HARD_KMEANS_1_SCORE,
            NSliceWebProduction::FI_DSSM_PANTHER_TERMS
        };

        webData.WebMetaFeaturesIds = {
            NSliceWebMeta::FI_META_NUM_URLS_PER_HOST_FIXED,
            NSliceWebMeta::FI_SD_PRS_PERCENT,
            NSliceWebMeta::FI_META_FULLURL_4GRAM_3HOSTMIN
        };

        for (const auto serializer : TYPES) {
            TString serialized = SerializeToBase64(webData, serializer);
            TWebData deserialized = DeserializeFromBase64(serialized);
            UNIT_ASSERT(WebDataEqual(webData, deserialized, 0.005));
        }
    }
};
