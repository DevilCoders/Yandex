#include <kernel/turbo/canonizer/canonizer.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NTurbo;

Y_UNIT_TEST_SUITE(TurboPagesCanonizerTests) {

    Y_UNIT_TEST(TestDeprecatedSaasCanonization) {
        UNIT_ASSERT_VALUES_EQUAL(NTurbo::CanonizeUrlForSaasDeprecatedLegacy("http://m.lenta.ru/"), "lenta.ru/");
        UNIT_ASSERT_VALUES_EQUAL(NTurbo::CanonizeUrlForSaasDeprecatedLegacy("https://www.m.lenta.ru/"), "lenta.ru/");
        UNIT_ASSERT_VALUES_EQUAL(NTurbo::CanonizeUrlForSaasDeprecatedLegacy("https://www.lenta.ru/?utm_referrer=https%3A%2F%2Fzen.yandex.com"), "lenta.ru/");
        UNIT_ASSERT_VALUES_EQUAL(NTurbo::CanonizeUrlForSaasDeprecatedLegacy("https://www.lenta.ru/?utm_referrer=https%3A%2F%2Fzen.yandex.com&id=1"), "lenta.ru/?id=1");
        UNIT_ASSERT_VALUES_EQUAL(NTurbo::CanonizeUrlForSaasDeprecatedLegacy("https://www.lenta.ru/?id=1&utm_referrer=https%3A%2F%2Fzen.yandex.com"), "lenta.ru/?id=1");
        UNIT_ASSERT_VALUES_EQUAL(NTurbo::CanonizeUrlForSaasDeprecatedLegacy("https://www.lenta.ru/?id=1&utm_referrer=https%3A%2F%2Fzen.yandex.com&id1=1"), "lenta.ru/?id=1&id1=1");
    }

}
