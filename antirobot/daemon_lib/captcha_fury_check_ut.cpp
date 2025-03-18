#include <library/cpp/testing/unittest/registar.h>

#include "captcha_fury_check.h"

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(CaptchaFuryCheck) {

Y_UNIT_TEST(ParseFuryResult) {
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":[]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(!err.Defined());
        UNIT_ASSERT_VALUES_EQUAL(categs, TVector<EFuryCategory>({}));
    }
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":[{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"captcha_robot\",\"value\":true}]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(!err.Defined());
        UNIT_ASSERT_VALUES_EQUAL(categs, TVector<EFuryCategory>({EFuryCategory::CaptchaRobot}));
    }
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":["
            "{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"captcha_robot\",\"value\":false}"
            "]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(!err.Defined());
        UNIT_ASSERT_VALUES_EQUAL(categs, TVector<EFuryCategory>({}));
    }
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":["
            "{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"captcha_robot\",\"value\":true},"
            "{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"some_categ\",\"value\":true}"
            "]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(!err.Defined());
        UNIT_ASSERT_VALUES_EQUAL(categs, TVector<EFuryCategory>({EFuryCategory::CaptchaRobot}));
    }
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":["
            "{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"captcha_robot\",\"value\":true},"
            "{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"degradation_web\",\"value\":true}"
            "]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(!err.Defined());
        UNIT_ASSERT_VALUES_EQUAL(categs, TVector<EFuryCategory>({EFuryCategory::CaptchaRobot, EFuryCategory::DegradationWeb}));
    }
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":["
            "{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"captcha_robot\",\"value\":true},"
            "{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"degradation_market\",\"value\":true}"
            "]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(!err.Defined());
        UNIT_ASSERT_VALUES_EQUAL(categs, TVector<EFuryCategory>({EFuryCategory::CaptchaRobot, EFuryCategory::DegradationMarket}));
    }
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":["
            "{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"captcha_robot\",\"value\":true},"
            "{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"degradation_uslugi\",\"value\":true}"
            "]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(!err.Defined());
        UNIT_ASSERT_VALUES_EQUAL(categs, TVector<EFuryCategory>({EFuryCategory::CaptchaRobot, EFuryCategory::DegradationUslugi}));
    }

    // errors:
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(err.Defined());
    }
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"qwe\":[]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(err.Defined());
    }
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":[{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"na_me\":\"captcha_robot\",\"value\":true}]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(err.Defined());
    }
    {
        auto res = ParseFuryResult("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":[{\"source\":\"antifraud\",\"subsource\":\"captcha\",\"entity\":\"key\",\"key\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\",\"name\":\"captcha_robot\",\"va_lue\":true}]}");
        TVector<EFuryCategory> categs;
        TError err = res.PutValueTo(categs);
        UNIT_ASSERT(err.Defined());
    }
}

}

}
