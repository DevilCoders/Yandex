#include <kernel/multilanguage_hosts/multilanguage_hosts.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TMultilangHostsTest) {
    Y_UNIT_TEST(TestPrefixes) {
//        UNIT_ASSERT_EQUAL(ContOf<TString>("@RU.")("@US.")("@TUR.")("@DE.")("@IT.")("@UA.").To<TVector<TString>>(),
//                          GetRobotMultilangPrefixes());
//        UNIT_ASSERT_EQUAL(ContOf<TString>("@ru.")("@us.")("@tur.")("@de.")("@it.")("@ua.").To<TVector<TString>>(),
//                          GetSearchMultilangPrefixes());

        UNIT_ASSERT_STRINGS_EQUAL("@tur.", GetSearchMultilangPrefix(ELR_TR));
        UNIT_ASSERT_STRINGS_EQUAL("@TUR.", GetRobotMultilangPrefix(ELR_TR));
    }

    Y_UNIT_TEST(TestIsMultilanguageHost) {
        UNIT_ASSERT(IsMultilanguageHost("@.yandex.ru"));
        UNIT_ASSERT(IsMultilanguageHost(TStringBuf("@.yandex.ru")));
        UNIT_ASSERT(IsMultilanguageHost("@x.yandex.ru"));
        UNIT_ASSERT(IsMultilanguageHost(TStringBuf("@x.yandex.ru")));
        UNIT_ASSERT(!IsMultilanguageHost("xxx"));
        UNIT_ASSERT(!IsMultilanguageHost(TStringBuf("xxx")));
        UNIT_ASSERT(!IsMultilanguageHost("x.ru"));
        UNIT_ASSERT(!IsMultilanguageHost(TStringBuf("x.ru")));
    }

    Y_UNIT_TEST(TestEncodeMultilanguageHost) {
        UNIT_ASSERT_STRINGS_EQUAL("@TUR.x", EncodeMultilanguageHost(ELR_TR, "x", ELR_RU));
        UNIT_ASSERT_STRINGS_EQUAL("x", EncodeMultilanguageHost(ELR_TR, "x", ELR_TR));
        UNIT_ASSERT_STRINGS_EQUAL("@tur.x", EncodeMultilanguageHost(LangRegion2Str(ELR_TR), "x"));

        UNIT_ASSERT_STRINGS_EQUAL("@TUR.yandex.ru", EncodeMultilanguageHost(ELR_TR, "yandex.ru", ELR_RU));
        UNIT_ASSERT_STRINGS_EQUAL("yandex.ru", EncodeMultilanguageHost(ELR_TR, "yandex.ru", ELR_TR));
        UNIT_ASSERT_STRINGS_EQUAL("@tur.yandex.ru", EncodeMultilanguageHost(LangRegion2Str(ELR_TR), "yandex.ru"));
    }

    void DoTestDecodeMultilanguageHost(const char* mlhost, TStringBuf exphost, TStringBuf explang, bool withsep) {

        UNIT_ASSERT_STRINGS_EQUAL(exphost, DecodeMultilanguageHost(mlhost));
        UNIT_ASSERT_STRINGS_EQUAL(explang, GetMultilanguagePrefix(mlhost, withsep));

        char lang[] = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
        char host[] = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
        DecodeMultilanguageHost(mlhost, host, lang, withsep);
        UNIT_ASSERT_STRINGS_EQUAL(exphost, host);
        UNIT_ASSERT_STRINGS_EQUAL(explang, lang);
    }

    Y_UNIT_TEST(TestDecodeMultilanguageHost) {
        DoTestDecodeMultilanguageHost("@x.y", "y", "x", false);
        DoTestDecodeMultilanguageHost("@x.y", "y", "@x.", true);

        DoTestDecodeMultilanguageHost("@x.y.z", "y.z", "x", false);
        DoTestDecodeMultilanguageHost("@x.y.z", "y.z", "@x.", true);

        DoTestDecodeMultilanguageHost("@ru.yandex.ru", "yandex.ru", "ru", false);
        DoTestDecodeMultilanguageHost("@ru.yandex.ru", "yandex.ru", "@ru.", true);
    }
};
