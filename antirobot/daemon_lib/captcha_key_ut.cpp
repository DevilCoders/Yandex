#include <library/cpp/testing/unittest/registar.h>

#include "captcha_key.h"

#include <antirobot/lib/keyring.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>

using namespace NAntiRobot;

Y_UNIT_TEST_SUITE(TTestCaptchaToken) {

Y_UNIT_TEST(TestTokenRandomCaptcha) {
    const ECaptchaType CAPTCHA_TYPES[] = {ECaptchaType::SmartCheckbox};

    for (size_t i = 0; i < Y_ARRAY_SIZE(CAPTCHA_TYPES); ++i) {
        TCaptchaToken token1(CAPTCHA_TYPES[i], "somereqid123");
        UNIT_ASSERT_NO_EXCEPTION(TCaptchaToken::Parse(token1.AsString));

        TCaptchaToken token2 = TCaptchaToken::Parse(token1.AsString);
        UNIT_ASSERT_C(token2.CaptchaType == CAPTCHA_TYPES[i], token1.AsString);
        UNIT_ASSERT_C((TInstant::Now() - token2.Timestamp).Seconds() <= 1, token1.AsString);
    }
}

Y_UNIT_TEST(TestInvalid1) {
    UNIT_ASSERT_EXCEPTION(TCaptchaToken::Parse("0699d9b69e5f1c5b86063f7ff0e789f5"),
                          yexception);
}

Y_UNIT_TEST(TestInvalid2) {
    UNIT_ASSERT_EXCEPTION(TCaptchaToken::Parse(TStringBuf()), yexception);
}

Y_UNIT_TEST(TestInvalid3) {
    UNIT_ASSERT_EXCEPTION(TCaptchaToken::Parse("1/e81b7ade32f213a8af613a122780bf39"),
                          yexception);
}

Y_UNIT_TEST(TestInvalid4) {
    UNIT_ASSERT_EXCEPTION(TCaptchaToken::Parse("1//e81b7ade32f213a8af613a122780bf39"),
                          yexception);
}

Y_UNIT_TEST(TestInvalid5) {
    UNIT_ASSERT_EXCEPTION(TCaptchaToken::Parse("1/123aaa/e81b7ade32f213a8af613a122780bf39"),
                          yexception);
}
}

class TTestCaptchaKeyParams : public TTestBase {
public:
    void SetUp() override {
        const TString testKey = "102c46d700bed5c69ed20b7473886468";
        TStringInput keys(testKey);
        TKeyRing::SetInstance(TKeyRing(keys));
    }
};

Y_UNIT_TEST_SUITE_IMPL(TTestCaptchaKey, TTestCaptchaKeyParams) {

Y_UNIT_TEST(TestValid) {
    constexpr TStringBuf host = "yandex.ru";
    const TCaptchaToken token(ECaptchaType::SmartCheckbox, "somereqid");

    const TCaptchaKey key1{"some_key", token, host};
    const TString keyStr = key1.ToString();

    UNIT_ASSERT_NO_EXCEPTION(TCaptchaKey::Parse(keyStr, host));
    const TCaptchaKey key2 = TCaptchaKey::Parse(keyStr, host);
    UNIT_ASSERT_EQUAL(key1.Token.AsString, key2.Token.AsString);
    UNIT_ASSERT_EQUAL(key1.ImageKey, key2.ImageKey);
    UNIT_ASSERT_EQUAL(key1.Host, key2.Host);
}

Y_UNIT_TEST(TestInvalid1) {
    static const TString INVALID_DATA[] = {
        // no '_'
        "SomeKey0/1411123058/e3060307aacd04583bb2e1b3ab8dd4374f7f80950a6bebe02d6acafee816b6d0",
        // no signature
        "SomeKey_0/1411123058/e3060307aacd04583bb2e1b3ab8dd437_",
        // just one '_'
        "SomeKey0/1411123058/e3060307aacd04583bb2e1b3ab8dd437_4f7f80950a6bebe02d6acafee816b6d0",
        // invalid token
        "SomeKey_0/1411123058/e3007aacd04583bb2e1b3ab8dd437_4f7f80950a6bebe02d6acafee816b6d0",
        // wrong signature
        "SomeKey_0/1411123058/e3060307aacd04583bb2e1b3ab8dd437_4f7f80950a6bebe02d6acafee816",
        // empty string
        "",
    };
    for (size_t i = 0; i < Y_ARRAY_SIZE(INVALID_DATA); ++i) {
        UNIT_ASSERT_EXCEPTION(TCaptchaKey::Parse(INVALID_DATA[i], "yandex.ru"),
                              TCaptchaKey::TParseError);
    }
}

Y_UNIT_TEST(TestDifferentDomain) {
    const TCaptchaToken token(ECaptchaType::SmartCheckbox, "somereqid");

    const TCaptchaKey keyRu{"some_key", token, "yandex.ru"};
    TString keyStr = keyRu.ToString();

    UNIT_ASSERT_EXCEPTION(TCaptchaKey::Parse(keyStr, "yandex.com"), TCaptchaKey::TParseError);
}

}
