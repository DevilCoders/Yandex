#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include "ar_utils.h"
#include "preview_uid.h"

using namespace NAntiRobot;

Y_UNIT_TEST_SUITE_IMPL(TTestPreviewUid, TTestBase) {
    Y_UNIT_TEST(GetPreviewAgentType) {
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("WhatsApp/2.20.52 i"_sb), EPreviewAgentType::OTHER);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("WhatsApp/3.0"_sb), EPreviewAgentType::OTHER);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("Viber/5.5.0.3531 CFNetwork/758.5.3 Darwin/15.6.0"_sb), EPreviewAgentType::OTHER);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("Viber"_sb), EPreviewAgentType::OTHER);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("Mozilla/5.0 (Windows NT 6.1; WOW64) SkypeUriPreview Preview/0.5"_sb), EPreviewAgentType::OTHER);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("Mozilla/5.0 (Windows; U; Windows NT 5.1; ru; rv:1.8.0.9) Gecko/20061206 Firefox/1.5.0.9"_sb), EPreviewAgentType::UNKNOWN);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36"_sb), EPreviewAgentType::UNKNOWN);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.122 YaBrowser/20.3.0.2220 Yowser/2.5 Safari/537.36"_sb), EPreviewAgentType::UNKNOWN);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_1) AppleWebKit/601.2.4 (KHTML, like Gecko) Version/9.0.1 Safari/601.2.4 facebookexternalhit/1.1 Facebot Twitterbot/1.0"_sb), EPreviewAgentType::UNKNOWN);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_1) AppleWebKit/601.2.4 (KHTML, like Gecko) Version/9.0.1 Safari/601.2.4 Twitterbot/1.0"_sb), EPreviewAgentType::UNKNOWN);
        UNIT_ASSERT_VALUES_EQUAL(GetPreviewAgentType("Twitterbot Facebot"_sb), EPreviewAgentType::UNKNOWN);
    }
}

