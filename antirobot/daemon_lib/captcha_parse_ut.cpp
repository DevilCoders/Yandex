#include <library/cpp/testing/unittest/registar.h>

#include "captcha_parse.h"

using namespace NAntiRobot;

TCaptcha ParseApiCaptchaResponse(TStringBuf jsonResponse) {
    TCaptcha captcha;
    TError err = TryParseApiCaptchaResponse(jsonResponse).PutValueTo(captcha);
    UNIT_ASSERT(!err.Defined());
    return captcha;
}

Y_UNIT_TEST_SUITE(CaptchaParse) {
    Y_UNIT_TEST(ImageOnly) {
        const TStringBuf captchaJson =
            "{\"imageurl\":\"http://u.captcha.yandex.net/image?key=30GV2oGQIaIAqLaSUz_xi0ng7UkaqqyB\","
            "\"token\":\"30GV2oGQIaIAqLaSUz_xi0ng7UkaqqyB\"}";

        TCaptcha captcha = ParseApiCaptchaResponse(captchaJson);

        UNIT_ASSERT_VALUES_EQUAL(captcha.Key, "30GV2oGQIaIAqLaSUz_xi0ng7UkaqqyB");
        UNIT_ASSERT_VALUES_EQUAL(captcha.ImageUrl, "http://u.captcha.yandex.net/image?key=30GV2oGQIaIAqLaSUz_xi0ng7UkaqqyB");
        UNIT_ASSERT_VALUES_EQUAL(captcha.VoiceUrl, "");
        UNIT_ASSERT_VALUES_EQUAL(captcha.VoiceIntroUrl, "");
    }

    Y_UNIT_TEST(ImageAndVoice) {
        const TStringBuf captchaJson =
            "{\"imageurl\":\"http://i.captcha.yandex.net/image?key=20844bLmOy8qZDo6T4YlsiWLX4cQozCe\","
            "\"voiceurl\":\"http://i.captcha.yandex.net/voice?key=20844bLmOy8qZDo6T4YlsiWLX4cQozCe\","
            "\"voiceintrourl\":\"http://i.captcha.yandex.net/static/intro-tr.mp3\","
            "\"token\":\"20844bLmOy8qZDo6T4YlsiWLX4cQozCe\"}";

        TCaptcha captcha = ParseApiCaptchaResponse(captchaJson);

        UNIT_ASSERT_VALUES_EQUAL(captcha.Key, "20844bLmOy8qZDo6T4YlsiWLX4cQozCe");
        UNIT_ASSERT_VALUES_EQUAL(captcha.ImageUrl, "http://i.captcha.yandex.net/image?key=20844bLmOy8qZDo6T4YlsiWLX4cQozCe");
        UNIT_ASSERT_VALUES_EQUAL(captcha.VoiceUrl, "http://i.captcha.yandex.net/voice?key=20844bLmOy8qZDo6T4YlsiWLX4cQozCe");
        UNIT_ASSERT_VALUES_EQUAL(captcha.VoiceIntroUrl, "http://i.captcha.yandex.net/static/intro-tr.mp3");
    }

    Y_UNIT_TEST(CompleteJson) {
        const TStringBuf captchaJson =
            "{\"imageurl\":\"https://ext.captcha.yandex.net/image?key=18SpcA6pPRNT7tnxW6qbq3U3FJe5xfd3\","
            "\"https\":1,"
            "\"voiceurl\":\"https://ext.captcha.yandex.net/voice?key=18SpcA6pPRNT7tnxW6qbq3U3FJe5xfd3\","
            "\"voiceintrourl\":\"https://ext.captcha.yandex.net/static/intro-ru.mp3\","
            "\"token\":\"18SpcA6pPRNT7tnxW6qbq3U3FJe5xfd3\","
            "\"json\":\"1\"}";

        TCaptcha captcha = ParseApiCaptchaResponse(captchaJson);

        UNIT_ASSERT_VALUES_EQUAL(captcha.Key, "18SpcA6pPRNT7tnxW6qbq3U3FJe5xfd3");
        UNIT_ASSERT_VALUES_EQUAL(captcha.ImageUrl, "https://ext.captcha.yandex.net/image?key=18SpcA6pPRNT7tnxW6qbq3U3FJe5xfd3");
        UNIT_ASSERT_VALUES_EQUAL(captcha.VoiceUrl, "https://ext.captcha.yandex.net/voice?key=18SpcA6pPRNT7tnxW6qbq3U3FJe5xfd3");
        UNIT_ASSERT_VALUES_EQUAL(captcha.VoiceIntroUrl, "https://ext.captcha.yandex.net/static/intro-ru.mp3");
    }

    Y_UNIT_TEST(IncorrectJson) {
        UNIT_ASSERT(TryParseApiCaptchaResponse("bla-bla-bla").HasError());
    }

    Y_UNIT_TEST(NoKey) {
        const TStringBuf captchaJson =
            "{\"imageurl\":\"http://i.captcha.yandex.net/image?key=20844bLmOy8qZDo6T4YlsiWLX4cQozCe\","
            "\"voiceurl\":\"http://i.captcha.yandex.net/voice?key=20844bLmOy8qZDo6T4YlsiWLX4cQozCe\","
            "\"voiceintrourl\":\"http://i.captcha.yandex.net/static/intro-tr.mp3\"}";
        UNIT_ASSERT(TryParseApiCaptchaResponse(captchaJson).HasError());
    }

    Y_UNIT_TEST(NoImageUrl) {
        const TStringBuf captchaJson =
            "{\"voiceurl\":\"http://i.captcha.yandex.net/voice?key=20844bLmOy8qZDo6T4YlsiWLX4cQozCe\","
            "\"voiceintrourl\":\"http://i.captcha.yandex.net/static/intro-tr.mp3\","
            "\"token\":\"20844bLmOy8qZDo6T4YlsiWLX4cQozCe\"}";
        UNIT_ASSERT(TryParseApiCaptchaResponse(captchaJson).HasError());
    }
}
