#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/string/cast.h>
#include <util/stream/output.h>
#include <util/string/vector.h>

#include "catfilter.h"

const TString find(const TCatFilter &cf, const TString &url) {
    TVector<TString> vs;

    TCatAttrsPtr cat = cf.Find(url.data());

    for (TCatAttrs::const_iterator it = cat->begin(); it != cat->end(); ++it) {
        vs.push_back(ToString(*it));
    }

    return JoinStrings(vs, " ");
}

Y_UNIT_TEST_SUITE(TCatFilterTest) {
    Y_UNIT_TEST(TheTest) {
        TCatFilter cf;
        cf.Load((GetArcadiaTestsData() + "/catfilter/filter.obj").data());
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "http://yandex.ru").data(), "1111 1234 4321");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "http://maps.yandex.ru").data(), "1111 1234 4321");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "http://maps.yandex.ru/smth").data(), "1111 1234 4321");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "https://maps.yandex.ru").data(), "555 88888 999999");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "https://maps.yandex.ru/smth").data(), "555 88888 999999");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "https://smth.maps.yandex.ru/smth").data(), "555 88888 999999");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "https://yandex.ru/").data(), "222 333 555");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "https://smth.yandex.ru/").data(), "222 333 555");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "https://google.ru/").data(), "");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "http://google.ru/").data(), "");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "https://yandex.ru/some%20thing").data(), "333");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "http://www.klerk.ru/Events/ind%20ex.php/%D2%E5%F1%F2").data(), "111");
        UNIT_ASSERT_STRINGS_EQUAL(find(cf, "http://www.klerk.ru/Events/ind%20ex.php").data(), "");
    }
}
