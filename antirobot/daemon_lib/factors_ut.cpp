#include <library/cpp/testing/unittest/registar.h>

#include "factors.h"
#include "fullreq_info.h"
#include "rps_filter.h"
#include "autoru_offer.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <util/generic/xrange.h>

using namespace NAntiRobot;

namespace {
    constexpr float EPS = 1e-9f;
    constexpr size_t rawFactorsCount = TRawFactors::Count();
    constexpr size_t factorsWithAggregationCount = TFactorsWithAggregation::RawFactorsWithAggregationCount();

    void AssertAllZeroes(const TRawFactors& factors) {
        for (float factor : factors.Factors) {
            UNIT_ASSERT_DOUBLES_EQUAL(factor, 0.0, EPS);
        }
    }

    void AssertAllZeroes(const TFactorsWithAggregation& factors) {
        AssertAllZeroes(factors.NonAggregatedFactors);

        for (const auto& rawFactors : factors.AggregatedFactors) {
            AssertAllZeroes(rawFactors);
        }
    }

    void AssertAllZeroes(const TAllFactors& factors) {
        for (const auto& factorsWithAggregation : factors.FactorsWithAggregation) {
            AssertAllZeroes(factorsWithAggregation);
        }
    }

}

Y_UNIT_TEST_SUITE(TRawFactors) {
    Y_UNIT_TEST(ZeroInit) {
        TRawFactors factors;

        AssertAllZeroes(factors);
    }

    Y_UNIT_TEST(SetBool) {
        TRawFactors factors;
        factors.SetBool(0, true);
        factors.SetBool(1, true);
        factors.SetBool(rawFactorsCount - 1, true);

        UNIT_ASSERT_DOUBLES_EQUAL(factors.Factors[0], 1.0, EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(factors.Factors[1], 1.0, EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(factors.Factors[rawFactorsCount - 1], 1.0, EPS);
    }

    Y_UNIT_TEST(FillFromFeatures) {
        class TTestRequest: public TRequest {
            void PrintData(IOutputStream&, bool /* forceMaskCookies */) const final {
            }
            void PrintHeaders(IOutputStream&, bool /* forceMaskCookies */) const final {
            }
            void SerializeTo(NCacheSyncProto::TRequest&) const final {
            }
        };
        static TReloadableData reloadable;
        reloadable.RequestClassifier.Set(CreateClassifierForTests());
        reloadable.GeoChecker.Set(TGeoChecker("geodata6-xurma.bin"));
        TTestRequest request = {};
        TTimeStatInfoVector EmptyVector = {};
        TTimeStats FakeStats{EmptyVector, ""};

        request.HostType = EHostType::HOST_MUSIC;
        request.ReqType = EReqType::REQ_IMAGES;
        constexpr TStringBuf ip = "192.168.0.1";
        request.RawAddr.FromString(ip);
        request.UserAddr.FromString(ip);

        TRpsFilter fakeFilter(0, TDuration::Zero(), TDuration::Zero());

        const TString uaData = NResource::Find("browser.xml");
        const TString uaProfiles = NResource::Find("profiles.xml");
        const TString uaExtra = NResource::Find("extra.xml");
        uatraits::detector detector(uaData.c_str(), uaData.length(), uaProfiles.c_str(), uaProfiles.length(), uaExtra.c_str(), uaExtra.length());
        const TAutoruOfferDetector offerDetector("salt");

        TRequestFeatures::TContext ctx = {
            &request,
            nullptr,
            &reloadable,
            &detector,
            &offerDetector,
            fakeFilter
        };
        TRequestFeatures rf(ctx, FakeStats, FakeStats);
        TRawFactors factors;

        factors.FillFromFeatures(rf);

        UNIT_ASSERT_DOUBLES_EQUAL(factors.Factors[F_REQ_TYPE_FIRST + request.ReqType], 1.0, EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(factors.Factors[F_HOST_TYPE_FIRST + request.HostType], 1.0, EPS);
    }

    Y_UNIT_TEST(Serialization) {
        TRawFactors serialized;

        for (float& factor : serialized.Factors) {
            factor = RandomNumber<float>();
        }

        TStringStream ss;
        TRawFactors deserialized;

        serialized.Save(&ss);
        deserialized.Load(&ss);

        for (auto i : xrange(rawFactorsCount)) {
            UNIT_ASSERT_DOUBLES_EQUAL(deserialized.Factors[i], serialized.Factors[i], EPS);
        }
    }
}

Y_UNIT_TEST_SUITE(TFactorsWithAggregation) {
    Y_UNIT_TEST(ZeroInit) {
        TFactorsWithAggregation factors;

        AssertAllZeroes(factors);
    }

    Y_UNIT_TEST(Count) {
        UNIT_ASSERT(factorsWithAggregationCount >= rawFactorsCount);
    }

    Y_UNIT_TEST(Aggregate) {
        constexpr float w[] = {0.1294494271f, 0.04515838623f, 0.01376730204f};
        constexpr float eps = 1e-9f;
        constexpr float x = 0.75f;

        TFactorsWithAggregation factors;
        factors.NonAggregatedFactors.Factors[0] = x;
        const auto aggregatedFactors = factors.AggregatedFactors;

        factors.Aggregate();

        UNIT_ASSERT_DOUBLES_EQUAL(aggregatedFactors[0].Factors[0], x * w[0], eps);
        UNIT_ASSERT_DOUBLES_EQUAL(aggregatedFactors[1].Factors[0], x * w[1], eps);
        UNIT_ASSERT_DOUBLES_EQUAL(aggregatedFactors[2].Factors[0], x * w[2], eps);

        factors.Aggregate();

        UNIT_ASSERT_DOUBLES_EQUAL(aggregatedFactors[0].Factors[0], x * w[0] * (1.0f - w[0]) + x * w[0], eps);
        UNIT_ASSERT_DOUBLES_EQUAL(aggregatedFactors[1].Factors[0], x * w[1] * (1.0f - w[1]) + x * w[1], eps);
        UNIT_ASSERT_DOUBLES_EQUAL(aggregatedFactors[2].Factors[0], x * w[2] * (1.0f - w[2]) + x * w[2], eps);

        factors.Aggregate();

        UNIT_ASSERT_DOUBLES_EQUAL(aggregatedFactors[0].Factors[0], (x * w[0] * (1.0f - w[0]) + x * w[0]) * (1.0f - w[0]) + x * w[0], eps);
        UNIT_ASSERT_DOUBLES_EQUAL(aggregatedFactors[1].Factors[0], (x * w[1] * (1.0f - w[1]) + x * w[1]) * (1.0f - w[1]) + x * w[1], eps);
        UNIT_ASSERT_DOUBLES_EQUAL(aggregatedFactors[2].Factors[0], (x * w[2] * (1.0f - w[2]) + x * w[2]) * (1.0f - w[2]) + x * w[2], eps);
    }

    Y_UNIT_TEST(Serialization) {
        TFactorsWithAggregation serialized;
        for (float& factor : serialized.NonAggregatedFactors.Factors) {
            factor = RandomNumber<float>();
        }

        for (TRawFactors& rawFactors : serialized.AggregatedFactors) {
            for (float& factor : rawFactors.Factors) {
                factor = RandomNumber<float>();
            }
        }

        TStringStream ss;
        TFactorsWithAggregation deserialized;

        serialized.Save(&ss);
        deserialized.Load(&ss, factorsWithAggregationCount);

        for (auto i : xrange(rawFactorsCount)) {
            UNIT_ASSERT_DOUBLES_EQUAL(deserialized.NonAggregatedFactors.Factors[i], serialized.NonAggregatedFactors.Factors[i], EPS);
        }
        for (auto i : xrange(Y_ARRAY_SIZE(serialized.AggregatedFactors))) {
            for (auto j : xrange(rawFactorsCount)) {
                UNIT_ASSERT_DOUBLES_EQUAL(deserialized.AggregatedFactors[i].Factors[j], serialized.AggregatedFactors[i].Factors[j], EPS);
            }
        }
    }

    Y_UNIT_TEST(GetFactor) {
        TFactorsWithAggregation factors;
        factors.NonAggregatedFactors.Factors[0] = 123;
        factors.AggregatedFactors[0].Factors[1] = 456;
        factors.AggregatedFactors[1].Factors[2] = 789;

        UNIT_ASSERT_DOUBLES_EQUAL(factors.GetFactor(0), 123, EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(factors.GetFactor(rawFactorsCount + 1), 456, EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(factors.GetFactor(rawFactorsCount * 2 + 2), 789, EPS);
    }

    Y_UNIT_TEST(FactorNameWithAggregationSuffix) {
        // Shorter name
        constexpr auto getFactorNameWithSuffix = [](size_t index) {
            return TFactorsWithAggregation::GetFactorNameWithAggregationSuffix(index);
        };

        UNIT_ASSERT_EXCEPTION(getFactorNameWithSuffix(factorsWithAggregationCount), yexception);

        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(0), "req_other");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(F_REQ_TYPE_LAST + 1), "host_other");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(F_HOST_TYPE_LAST + 1), "rep_other");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(F_REPORT_TYPE_LAST + 1), "suid_spravka");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(F_HTTP_HEADER_PRESENCE_FIRST), "has_http_header_Accept");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(F_COOKIE_PRESENCE_FIRST), "has_cookie_fyandex");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(F_CGI_PARAM_FIRST), "cgi_text");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(F_PERSON_LANG_FIRST), "plang_unk");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(F_QUERY_LANG_FIRST), "qlang_rus_mixed");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(F_QUERY_CLASS_FIRST), "class_download");

        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(rawFactorsCount + 1), "req_main^5");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(rawFactorsCount * 2 + 2), "req_ys^15");
        UNIT_ASSERT_STRINGS_EQUAL(getFactorNameWithSuffix(rawFactorsCount * 3 + 3), "req_xml^50");
    }
}

Y_UNIT_TEST_SUITE(TAllFactors) {
    Y_UNIT_TEST(ZeroInit) {
        TAllFactors factors;

        AssertAllZeroes(factors);
    }

    Y_UNIT_TEST(Linearize) {
        TAllFactors factors;

        factors.FactorsWithAggregation[0].NonAggregatedFactors.Factors[1] = 1;
        factors.FactorsWithAggregation[0].AggregatedFactors[0].Factors[1] = 2;

        factors.FactorsWithAggregation[1].NonAggregatedFactors.Factors[2] = 3;
        factors.FactorsWithAggregation[1].AggregatedFactors[1].Factors[2] = 4;

        factors.FactorsWithAggregation[2].NonAggregatedFactors.Factors[3] = 5;
        factors.FactorsWithAggregation[2].AggregatedFactors[2].Factors[3] = 6;

        factors.FactorsWithAggregation[3].NonAggregatedFactors.Factors[4] = 7;
        factors.FactorsWithAggregation[3].AggregatedFactors[2].Factors[4] = 8;

        TProcessorLinearizedFactors linearizedFactors = factors.GetLinearizedFactors();

        UNIT_ASSERT_VALUES_EQUAL(linearizedFactors.size(), TAllFactors::AllFactorsCount());

        UNIT_ASSERT_DOUBLES_EQUAL(linearizedFactors[1], 1, EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(linearizedFactors[rawFactorsCount + 1], 2, EPS);

        UNIT_ASSERT_DOUBLES_EQUAL(linearizedFactors[factorsWithAggregationCount + 2], 3, EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(linearizedFactors[factorsWithAggregationCount + rawFactorsCount * 2 + 2], 4, EPS);

        UNIT_ASSERT_DOUBLES_EQUAL(linearizedFactors[factorsWithAggregationCount * 2 + 3], 5, EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(linearizedFactors[factorsWithAggregationCount * 2 + rawFactorsCount * 3 + 3], 6, EPS);

        UNIT_ASSERT_DOUBLES_EQUAL(linearizedFactors[factorsWithAggregationCount * 3 + 4], 7, EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(linearizedFactors[factorsWithAggregationCount * 3 + rawFactorsCount * 3 + 4], 8, EPS);
    }

    Y_UNIT_TEST(Count) {
        UNIT_ASSERT(TAllFactors::AllFactorsCount() >= factorsWithAggregationCount);
    }

    Y_UNIT_TEST(FactorName) {
        UNIT_ASSERT_EXCEPTION(TAllFactors::GetFactorNameByIndex(TAllFactors::AllFactorsCount()), yexception);

        UNIT_ASSERT_STRINGS_EQUAL(TAllFactors::GetFactorNameByIndex(0), "req_other");
        UNIT_ASSERT_STRINGS_EQUAL(TAllFactors::GetFactorNameByIndex(factorsWithAggregationCount + rawFactorsCount * 2), "req_other^15|ip");
        UNIT_ASSERT_STRINGS_EQUAL(TAllFactors::GetFactorNameByIndex(factorsWithAggregationCount * 2 + rawFactorsCount * 3), "req_other^50|C");
    }
}
