#include <library/cpp/testing/unittest/registar.h>

#include "catboost.h"
#include "cacher_factors.h"

namespace NAntiRobot {


Y_UNIT_TEST_SUITE(TTestCatboost) {
    const float DEFAULT_EPS = 1e-4;

    Y_UNIT_TEST(TestLoadFromula) {
        TCatboostClassificator<TCacherLinearizedFactors> classif("catboost.info");
    }

    Y_UNIT_TEST(TestLoadFromulaNotExits) {
        UNIT_ASSERT_EXCEPTION(TCatboostClassificator<TCacherLinearizedFactors>("catboost_none.info"), yexception);
    }

    Y_UNIT_TEST(TestClassify) {
        TCatboostClassificator<TCacherLinearizedFactors> classif("catboost.info");
        TRawCacherFactors cacherRaw;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::NumDocs)] = 10;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::PageNum)] = 0;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeadersCount)] = 20;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::IsProxy)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::IsTor)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::IsVpn)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::IsHosting)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::RefererFromYandex)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::IsBadProtocol)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::IsBadUserAgent)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::IsConnectionKeepAlive)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HaveUnknownHeaders)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::IpSubnetMatch)] = 0;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::SpravkaLifetime)] = 0.0;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::FraudJa3)] = std::numeric_limits<float>::quiet_NaN();
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::FraudSubnet)] = 60.0;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CgiParamText)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CgiParamLr)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CgiParamClid)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CgiParamTld)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CgiParamUrl)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CgiParamSite)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CgiParamLang)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CookieL)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CookieMy)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CookieSessionId)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CookieYabsFrequency)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CookieYandexLogin)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CookieYandexuid)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CookieYs)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::CookieFuid01)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderAcceptEncoding)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderAcceptLanguage)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderAuthorization)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderCacheControl)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderCookie)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderConnection)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderContentLength)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderContentType)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderReferer)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderUserAgent)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HeaderXForwardedFor)] = true;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::InRobotSet)] = false;
		cacherRaw.Factors[static_cast<size_t>(ECacherFactors::HasValidSpravka)] = false;

        UNIT_ASSERT_DOUBLES_EQUAL(classif(cacherRaw.Factors), 4.1814470291137695, DEFAULT_EPS);
    }
}

} // namespace NAntiRobot


