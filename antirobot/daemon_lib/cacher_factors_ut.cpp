#include "cacher_factors.h"
#include "request_features.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/xrange.h>

namespace NAntiRobot {

class TTestCacherFacherFactors : public TTestBase {
public:
    TString Name() const noexcept override {
        return "TTestCacherFacherFactors";
    }

    void SetUp() override {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString("<Daemon>\n"
                                                       "AuthorizeByFuid = 1\n"
                                                       "AuthorizeByICookie = 1\n"
                                                       "AuthorizeByLCookie = 1\n"
                                                       "CaptchaApiHost = ::\n"
                                                       "FormulasDir = .\n"
                                                       "CbbApiHost = ::\n"
                                                       "</Daemon>\n"
                                                       "<Zone></Zone>");
        TJsonConfigGenerator jsonConf;
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.JsonConfig.LoadFromString(jsonConf.Create(), GetJsonServiceIdentifierStr());

    }
};

Y_UNIT_TEST_SUITE_IMPL(TestCacherFactors, TTestCacherFacherFactors) {

    void CheckSingleFactor(
        const TString& req, ECacherFactors factorId, float value, const TVector<ECacherFactors>& exclude,
        const TVector<ECacherFactors>& dicts = {
            ECacherFactors::FraudJa3,
            ECacherFactors::FraudSubnet,
            ECacherFactors::FraudSubnetNew,
            ECacherFactors::P0fITTLDistance,
            ECacherFactors::P0fMSS,
            ECacherFactors::P0fWSize,
            ECacherFactors::P0fScale,
            ECacherFactors::P0fEOL,
            ECacherFactors::AcceptUniqueKeysNumber,
            ECacherFactors::AcceptEncodingUniqueKeysNumber,
            ECacherFactors::AcceptCharsetUniqueKeysNumber,
            ECacherFactors::AcceptLanguageUniqueKeysNumber,
            ECacherFactors::AcceptAnySpace,
            ECacherFactors::AcceptEncodingAnySpace,
            ECacherFactors::AcceptCharsetAnySpace,
            ECacherFactors::AcceptLanguageAnySpace,
            ECacherFactors::AcceptLanguageHasRussian,
            ECacherFactors::P0fUnknownOptionID,
            ECacherFactors::MarketJwsStateIsDefaultExpiredRatio,
            ECacherFactors::MarketJwsStateIsDefaultRatio,
            ECacherFactors::MarketJwsStateIsInvalidRatio,
            ECacherFactors::MarketJwsStateIsSuspExpiredRatio,
            ECacherFactors::MarketJwsStateIsSuspRatio,
            ECacherFactors::MarketJwsStateIsValidExpiredRatio,
            ECacherFactors::MarketJwsStateIsValidRatio,
            ECacherFactors::MarketJa3BlockedCntRatio,
            ECacherFactors::MarketJa3CatalogReqsCntRatio,
            ECacherFactors::MarketJa3EnemyCntRatio,
            ECacherFactors::MarketJa3EnemyRedirectsCntRatio,
            ECacherFactors::MarketJa3FuidCntRatio,
            ECacherFactors::MarketJa3HostingCntRatio,
            ECacherFactors::MarketJa3IcookieCntRatio,
            ECacherFactors::MarketJa3Ipv4CntRatio,
            ECacherFactors::MarketJa3Ipv6CntRatio,
            ECacherFactors::MarketJa3LoginCntRatio,
            ECacherFactors::MarketJa3MobileCntRatio,
            ECacherFactors::MarketJa3OtherHandlesReqsCntRatio,
            ECacherFactors::MarketJa3ProductReqsCntRatio,
            ECacherFactors::MarketJa3ProxyCntRatio,
            ECacherFactors::MarketJa3RefererIsEmptyCntRatio,
            ECacherFactors::MarketJa3RefererIsNotYandexCntRatio,
            ECacherFactors::MarketJa3RefererIsYandexCntRatio,
            ECacherFactors::MarketJa3RobotsCntRatio,
            ECacherFactors::MarketJa3SearchReqsCntRatio,
            ECacherFactors::MarketJa3SpravkaCntRatio,
            ECacherFactors::MarketJa3TorCntRatio,
            ECacherFactors::MarketJa3VpnCntRatio,
            ECacherFactors::MarketJa3YndxIpCntRatio,
            ECacherFactors::MarketSubnetBlockedCntRatio,
            ECacherFactors::MarketSubnetCatalogReqsCntRatio,
            ECacherFactors::MarketSubnetEnemyCntRatio,
            ECacherFactors::MarketSubnetEnemyRedirectsCntRatio,
            ECacherFactors::MarketSubnetFuidCntRatio,
            ECacherFactors::MarketSubnetHostingCntRatio,
            ECacherFactors::MarketSubnetIcookieCntRatio,
            ECacherFactors::MarketSubnetIpv4CntRatio,
            ECacherFactors::MarketSubnetIpv6CntRatio,
            ECacherFactors::MarketSubnetLoginCntRatio,
            ECacherFactors::MarketSubnetMobileCntRatio,
            ECacherFactors::MarketSubnetOtherHandlesReqsCntRatio,
            ECacherFactors::MarketSubnetProductReqsCntRatio,
            ECacherFactors::MarketSubnetProxyCntRatio,
            ECacherFactors::MarketSubnetRefererIsEmptyCntRatio,
            ECacherFactors::MarketSubnetRefererIsNotYandexCntRatio,
            ECacherFactors::MarketSubnetRefererIsYandexCntRatio,
            ECacherFactors::MarketSubnetRobotsCntRatio,
            ECacherFactors::MarketSubnetSearchReqsCntRatio,
            ECacherFactors::MarketSubnetSpravkaCntRatio,
            ECacherFactors::MarketSubnetTorCntRatio,
            ECacherFactors::MarketSubnetVpnCntRatio,
            ECacherFactors::MarketSubnetYndxIpCntRatio,
            ECacherFactors::MarketUABlockedCntRatio,
            ECacherFactors::MarketUACatalogReqsCntRatio,
            ECacherFactors::MarketUAEnemyCntRatio,
            ECacherFactors::MarketUAEnemyRedirectsCntRatio,
            ECacherFactors::MarketUAFuidCntRatio,
            ECacherFactors::MarketUAHostingCntRatio,
            ECacherFactors::MarketUAIcookieCntRatio,
            ECacherFactors::MarketUAIpv4CntRatio,
            ECacherFactors::MarketUAIpv6CntRatio,
            ECacherFactors::MarketUALoginCntRatio,
            ECacherFactors::MarketUAMobileCntRatio,
            ECacherFactors::MarketUAOtherHandlesReqsCntRatio,
            ECacherFactors::MarketUAProductReqsCntRatio,
            ECacherFactors::MarketUAProxyCntRatio,
            ECacherFactors::MarketUARefererIsEmptyCntRatio,
            ECacherFactors::MarketUARefererIsNotYandexCntRatio,
            ECacherFactors::MarketUARefererIsYandexCntRatio,
            ECacherFactors::MarketUARobotsCntRatio,
            ECacherFactors::MarketUASearchReqsCntRatio,
            ECacherFactors::MarketUASpravkaCntRatio,
            ECacherFactors::MarketUATorCntRatio,
            ECacherFactors::MarketUAVpnCntRatio,
            ECacherFactors::MarketUAYndxIpCntRatio,
            ECacherFactors::AutoruJa3,
            ECacherFactors::AutoruSubnet,
            ECacherFactors::AutoruUA,
            ECacherFactors::CookieYoungerThanMinute,
            ECacherFactors::CookieYoungerThanHour,
            ECacherFactors::CookieYoungerThanDay,
            ECacherFactors::CookieYoungerThanWeek,
            ECacherFactors::CookieYoungerThanMonth,
            ECacherFactors::CookieYoungerThanThreeMonthes,
            ECacherFactors::CookieOlderThanMonth,
            ECacherFactors::CookieOlderThanThreeMonthes,
        }
    ) {
        auto rf = MakeCacherReqFeatures(req);

        TRawCacherFactors cf;
        cf.FillCacherRequestFeatures(rf);

        for (const auto i : xrange(static_cast<size_t>(ECacherFactors::NumStaticFactors))) {
            const auto name = static_cast<ECacherFactors>(i);
            TStringStream ss;
            ss << "Check factor: " << name << " failed!";
            if (name == factorId) {
                UNIT_ASSERT_EQUAL_C(cf.Factors[i], value, ss.Str());
            } else if (!IsIn(exclude, name)) {
                if (IsIn(dicts, name)) {
                    UNIT_ASSERT_C(std::isnan(cf.Factors[i]), ss.Str());
                } else {
                    UNIT_ASSERT_EQUAL_C(cf.Factors[i], 0.0f, ss.Str());
                }
            }
        }
    }

    Y_UNIT_TEST(TestEmptyFactors) {
        TRawCacherFactors raw;

        UNIT_ASSERT_VALUES_EQUAL(
            raw.Factors.size(),
            static_cast<size_t>(ECacherFactors::NumStaticFactors) +
                ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.LastVisitsRules.size()
        );

        for (auto f : raw.Factors) {
            UNIT_ASSERT_EQUAL(f, 0.0f);
        }
    }

    Y_UNIT_TEST(TestNumDocs) {
        TString req =
            "GET /foo?numdoc=12 HTTP/1.1\r\n"
            "X-Forwarded-For-Y: 81.18.114.137\r\n"
            "\r\n";

        TVector excludeList = {
            ECacherFactors::IsBadUserAgent,
            ECacherFactors::IsBadUserAgentNew,
            ECacherFactors::IsConnectionKeepAlive,
        };

        CheckSingleFactor(req, ECacherFactors::NumDocs, 12.f, excludeList);
    }

    Y_UNIT_TEST(TestPageNum) {
        TString req =
            "GET /foo?p=42 HTTP/1.1\r\n"
            "X-Forwarded-For-Y: 81.18.114.137\r\n"
            "\r\n";

        TVector excludeList = {
            ECacherFactors::IsBadUserAgent,
            ECacherFactors::IsBadUserAgentNew,
            ECacherFactors::IsConnectionKeepAlive,
            ECacherFactors::NumDocs,
        };
        CheckSingleFactor(req, ECacherFactors::PageNum, 42.f, excludeList);
    }

    Y_UNIT_TEST(TestHeadersCount) {
        TString req =
            "GET /foo HTTP/1.1\r\n"
            "X-Forwarded-For-Y: 81.18.114.137\r\n"
            "Host: yandex.ru\r\n"
            "If-Match: blabla\r\n"
            "\r\n";

        TVector excludeList = {
            ECacherFactors::IsBadUserAgent,
            ECacherFactors::IsBadUserAgentNew,
            ECacherFactors::IsConnectionKeepAlive,
            ECacherFactors::NumDocs,
        };
        CheckSingleFactor(req, ECacherFactors::HeadersCount, 2.f, excludeList);
    }

    Y_UNIT_TEST(TestRefererFromYandex) {
        TString req =
            "GET /foo HTTP/1.1\r\n"
            "X-Forwarded-For-Y: 81.18.114.137\r\n"
            "Host: yandex.ru\r\n"
            "Referer: http://yandex.ru/foo/bar\r\n"
            "If-Match: blabla\r\n"
            "\r\n";

        TVector excludeList = {
            ECacherFactors::HeaderReferer,
            ECacherFactors::HeadersCount,
            ECacherFactors::IsBadUserAgent,
            ECacherFactors::IsBadUserAgentNew,
            ECacherFactors::IsConnectionKeepAlive,
            ECacherFactors::NumDocs,
            ECacherFactors::RefererFromYandex,
        };
        CheckSingleFactor(req, ECacherFactors::RefererFromYandex, 1.f, excludeList);
        CheckSingleFactor(req, ECacherFactors::HeaderReferer, 1.f, excludeList);
    }

    // TODO: add more tests for other factors
}

} // namespace NAntiRobot
