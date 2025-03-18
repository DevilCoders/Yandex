#include <library/cpp/deprecated/base64/yabs/base64.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(Base64) {
    Y_UNIT_TEST(EncodeBufferOverflowCheckExact) {
        unsigned char const buf[1] = { '0' };
        char str[2];
        UNIT_ASSERT_VALUES_EQUAL(YabsSpylogBase64Encode(buf, sizeof(buf), str, sizeof(str)), sizeof(str));
    }

    Y_UNIT_TEST(DecodeBufferOverflowCheckExact) {
        char const str[2] = { 'm', '0' };
        unsigned char buf[1];
        bool failed;
        YabsBase64Decode(str, sizeof(str), buf, sizeof(buf), failed);
        UNIT_ASSERT_VALUES_EQUAL(failed, false);
    }
}
