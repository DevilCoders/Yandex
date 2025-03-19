#include <kernel/relev_locale/serptld.h>

#include <util/generic/string.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TYandexSerpTldTests) {
    Y_UNIT_TEST(TestFromString) {
        UNIT_ASSERT_VALUES_EQUAL(YST_RU, FromString<EYandexSerpTld>("ru"));
        UNIT_ASSERT_VALUES_EQUAL(YST_UA, FromString<EYandexSerpTld>("ua"));
        UNIT_ASSERT_VALUES_EQUAL(YST_BY, FromString<EYandexSerpTld>("by"));
        UNIT_ASSERT_VALUES_EQUAL(YST_KZ, FromString<EYandexSerpTld>("kz"));
        UNIT_ASSERT_VALUES_EQUAL(YST_UZ, FromString<EYandexSerpTld>("uz"));
        UNIT_ASSERT_VALUES_EQUAL(YST_TR, FromString<EYandexSerpTld>("tr"));
        UNIT_ASSERT_VALUES_EQUAL(YST_IL, FromString<EYandexSerpTld>("il"));
        UNIT_ASSERT_VALUES_EQUAL(YST_COM, FromString<EYandexSerpTld>("com"));
        UNIT_ASSERT_VALUES_EQUAL(YST_FI, FromString<EYandexSerpTld>("fi"));
        UNIT_ASSERT_VALUES_EQUAL(YST_PL, FromString<EYandexSerpTld>("pl"));
        UNIT_ASSERT_VALUES_EQUAL(YST_EU, FromString<EYandexSerpTld>("eu"));

        UNIT_ASSERT_VALUES_EQUAL(YST_TR, FromString<EYandexSerpTld>("com.tr"));
        UNIT_ASSERT_VALUES_EQUAL(YST_IL, FromString<EYandexSerpTld>("co.il"));
        UNIT_ASSERT_VALUES_EQUAL(YST_KZ, FromString<EYandexSerpTld>("xn--p1ai"));

        UNIT_ASSERT_VALUES_EQUAL(YST_UNKNOWN, FromString<EYandexSerpTld>("hello, there"));
        UNIT_ASSERT_VALUES_EQUAL(YST_RU, FromString<EYandexSerpTld>(""));
    }

    Y_UNIT_TEST(TestToString) {
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_BY), "by");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_KZ), "kz");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_RU), "ru");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_TR), "tr");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_IL), "il");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_UA), "ua");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_UZ), "uz");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_COM), "com");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_FI), "fi");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_PL), "pl");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_EU), "eu");
        UNIT_ASSERT_VALUES_EQUAL(ToString(YST_UNKNOWN), "UNKNOWN");
    }

    Y_UNIT_TEST(TestIsCIS) {
        UNIT_ASSERT(IsCISSerp(YST_BY));
        UNIT_ASSERT(IsCISSerp(YST_KZ));
        UNIT_ASSERT(IsCISSerp(YST_RU));
        UNIT_ASSERT(IsCISSerp(YST_UA));
        UNIT_ASSERT(IsCISSerp(YST_UZ));
        UNIT_ASSERT(!IsCISSerp(YST_TR));
        UNIT_ASSERT(!IsCISSerp(YST_UNKNOWN));
    }

    Y_UNIT_TEST(TestFromStringToString) {
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("")), "ru");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("ru")), "ru");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("ua")), "ua");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("com")), "com");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("am")), "am");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("com.am")), "am");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("il")), "il");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("co.il")), "il");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("fi")), "fi");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("pl")), "pl");
        UNIT_ASSERT_VALUES_EQUAL(ToString(FromString<EYandexSerpTld>("UNKNOWN")), "UNKNOWN");
    }

    Y_UNIT_TEST(TestToStringFromString) {
        UNIT_ASSERT_VALUES_EQUAL(FromString<EYandexSerpTld>(ToString(YST_RU)), YST_RU);
        UNIT_ASSERT_VALUES_EQUAL(FromString<EYandexSerpTld>(ToString(YST_UA)), YST_UA);
        UNIT_ASSERT_VALUES_EQUAL(FromString<EYandexSerpTld>(ToString(YST_COM)), YST_COM);
        UNIT_ASSERT_VALUES_EQUAL(FromString<EYandexSerpTld>(ToString(YST_AM)), YST_AM);
        UNIT_ASSERT_VALUES_EQUAL(FromString<EYandexSerpTld>(ToString(YST_IL)), YST_IL);
        UNIT_ASSERT_VALUES_EQUAL(FromString<EYandexSerpTld>(ToString(YST_FI)), YST_FI);
        UNIT_ASSERT_VALUES_EQUAL(FromString<EYandexSerpTld>(ToString(YST_PL)), YST_PL);
        UNIT_ASSERT_VALUES_EQUAL(FromString<EYandexSerpTld>(ToString(YST_UNKNOWN)), YST_UNKNOWN);
    }

    Y_UNIT_TEST(TestToStringFromStringToString) {
        for (int i = EYandexSerpTld_MIN; i < EYandexSerpTld_ARRAYSIZE; ++i) {
            if (!EYandexSerpTld_IsValid(i)) {
                 continue;
            }

            UNIT_ASSERT_VALUES_EQUAL(
                ToString(static_cast<EYandexSerpTld>(i)),
                ToString(FromString<EYandexSerpTld>(ToString(static_cast<EYandexSerpTld>(i))))
            );

            UNIT_ASSERT_VALUES_EQUAL(i, FromString<EYandexSerpTld>(ToString(static_cast<EYandexSerpTld>(i))));
        }
    }
    Y_UNIT_TEST(TestToDomainsFromId) {
        UNIT_ASSERT_VALUES_EQUAL(GetYandexDomainBySerpTld(YST_RU), "ru");
        UNIT_ASSERT_VALUES_EQUAL(GetYandexDomainBySerpTld(YST_TR), "com.tr");
        UNIT_ASSERT_VALUES_EQUAL(GetYandexDomainBySerpTld(YST_IL), "co.il");
    }

}

