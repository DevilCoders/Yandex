#include <library/cpp/testing/unittest/registar.h>
#include "clon.h"

template<class T>
static TString V2S(const TVector<T>& v) {
    if (v.empty())
        return "";
    TString r = ToString(v[0]);
    for (size_t i = 1; i < v.size(); ++i)
        r += TString(",") + ToString(v[i]);
    return r;
}

Y_UNIT_TEST_SUITE(TClonCanonizerTest) {
    Y_UNIT_TEST(TheTest) {
        TClonAttrCanonizer cac;
        cac.AddClon("yandex.ru", 1, 1);
        cac.AddClon("222.yandex.ru", 2, 1);
        cac.AddClon("333.yandex.ru", 1, 2);
        cac.AddClon("yandex.net", 3, 3);
        cac.AddClon("yandex.net", 4, 1);
        cac.AddClon("www.yandex.eu", 1, 1);
        cac.AddClon("www.yandex.eu", 2, 1);
        cac.AddClon("www.yandex.eu", 3, 1);
        cac.AddClon("www.yandex.eu", 9999, 1);
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("yandex.ru")).data(), "1");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("111.yandex.ru")).data(), "1");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("222.yandex.ru")).data(), "1,2");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("www.222.yandex.ru")).data(), "1,2");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("333.yandex.ru")).data(), "");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("www.333.yandex.ru")).data(), "1");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("yandex.net")).data(), "3,4");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("111.yandex.net")).data(), "4");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("0.1.2.3.111.yandex.net")).data(), "4");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("yandex.eu")).data(), "");
        UNIT_ASSERT_STRINGS_EQUAL(V2S(cac.GetHostClonGroups("www.yandex.eu")).data(), "1,2,3,9999");
    }
}
