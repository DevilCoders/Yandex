#include "relev_locale.h"

#include <kernel/country_data/countries.h>

#include <library/cpp/testing/unittest/registar.h>

#include <utility>
#include <library/cpp/langs/langs.h>
#include <util/generic/algorithm.h>
#include <util/generic/cast.h>
#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <util/stream/output.h>

using namespace NRl;


Y_UNIT_TEST_SUITE(TRelevLocaleTests) {
    Y_UNIT_TEST(TestIfEnumerationValiesAreConsecutive) {
        for (int index = 0; index < ERelevLocale_ARRAYSIZE; ++index) {
            UNIT_ASSERT(ERelevLocale_IsValid(index));
        }
    }

    Y_UNIT_TEST(TestArcadiaStyleStringCastEqualToProtobufStringCastSuffix) {
        for (int index = 0; index < ERelevLocale_ARRAYSIZE; ++index) {
            const ERelevLocale relevLocale = static_cast<ERelevLocale>(index);
            const TString protobufCasted = ERelevLocale_Name(relevLocale);
            const size_t underlineLastPos = protobufCasted.find_last_of('_');
            UNIT_ASSERT(TString::npos != underlineLastPos);

            const TString protobufCastedSuffix =
                TString{TStringBuf(protobufCasted).SubStr(underlineLastPos + 1)};

            UNIT_ASSERT_VALUES_EQUAL(
                to_lower(protobufCastedSuffix),
                to_lower(ToString(relevLocale))
            );
        }
    }

    Y_UNIT_TEST(TestFromString) {
        const std::pair<TStringBuf, ERelevLocale> validStrings[] = {
            { "universe", RL_UNIVERSE },
            { "unknown", RL_UNIVERSE },
            { "ru", RL_RU },
            { "ua", RL_UA },
            { "uz", RL_UZ },
            { "gb", RL_GB },
            { "il", RL_IL },
            { "ge", RL_GE },
            { "world", RL_WORLD },
            { "xussr", RL_XUSSR },
            { "xcom", RL_XCOM },
            { "spok", RL_SPOK },
        };

        for (const auto& test : validStrings) {
            UNIT_ASSERT_VALUES_EQUAL(test.second, ERelevLocaleFromString(test.first));

            ERelevLocale relevLocale = FromString(test.first);
            UNIT_ASSERT_VALUES_EQUAL(test.second, relevLocale);
        }

        const TStringBuf invalidStrings[] = {
            "",
            "Ru",
            "RU",
            "en",
            "co.il",
            "XCOM",
            "shit",
        };

        for (const TStringBuf& invalid : invalidStrings) {
            UNIT_ASSERT_EXCEPTION(ERelevLocaleFromString(invalid), yexception);

            ERelevLocale result;
            UNIT_ASSERT_EXCEPTION(result = FromString(invalid), yexception);
            UNIT_ASSERT(!FromString(invalid, result));
        }
    }

    Y_UNIT_TEST(TestParentChildrenConsistensy) {
        for (int index = 0; index < ERelevLocale_ARRAYSIZE; ++index) {
            const ERelevLocale parent = static_cast<ERelevLocale>(index);

            TVector<ERelevLocale> childrenExpected;
            for (int jindex = 0; jindex < ERelevLocale_ARRAYSIZE; ++jindex) {
                const ERelevLocale relevLocale = static_cast<ERelevLocale>(jindex);

                if (parent == GetLocaleParent(relevLocale)) {
                    childrenExpected.push_back(relevLocale);
                    if (RL_UNIVERSE == parent && RL_UNIVERSE == relevLocale)
                        childrenExpected.pop_back();
                }
            }

            Sort(childrenExpected.begin(), childrenExpected.end());
            UNIT_ASSERT_VALUES_EQUAL(childrenExpected, GetLocaleChildren(parent));
        }

        for (int index = 0; index < ERelevLocale_ARRAYSIZE; ++index) {
            const ERelevLocale parent = static_cast<ERelevLocale>(index);

            for (const auto& child: GetLocaleChildren(parent)) {
                UNIT_ASSERT_VALUES_EQUAL(parent, GetLocaleParent(child));
            }
        }
    }

    Y_UNIT_TEST(TestBasicLocales) {
        const auto& basicLocales = GetBasicLocales();

        for (const auto relevLocale: basicLocales)
            UNIT_ASSERT(ERelevLocale_IsValid(relevLocale));

        for (const ERelevLocale relevLocale: GetAllLocales()) {
            if (GetLocaleChildren(relevLocale).empty()) {
                UNIT_ASSERT(Find(basicLocales.begin(), basicLocales.end(), relevLocale) != basicLocales.end());
            }
        }
    }

    void CheckRelevLocaleDescendants(ERelevLocale rl, ERelevLocale parent) {
        for (ERelevLocale child: GetLocaleChildren(rl)) {
            UNIT_ASSERT(IsLocaleDescendantOf(child, parent));
            CheckRelevLocaleDescendants(child, parent);
        }
    }

    Y_UNIT_TEST(TestIsRelevLocaleDescendantOf) {
        for (int index = 0; index < ERelevLocale_ARRAYSIZE; ++index) {
            ERelevLocale parent = ERelevLocale(index);
            UNIT_ASSERT(IsLocaleDescendantOf(parent, parent));
            CheckRelevLocaleDescendants(parent, parent);
        }
    }

    void CheckRelevLocaleDescendantsWithLevel(ERelevLocale rl, ERelevLocale parent) {
        for (ERelevLocale child: GetLocaleChildren(rl)) {
            size_t level;
            UNIT_ASSERT(IsLocaleDescendantOf(child, parent, &level));

            ERelevLocale childsParent = child;
            size_t expectedLevel = 0;
            while (childsParent != parent) {
                childsParent = GetLocaleParent(childsParent);
                ++expectedLevel;
            }
            UNIT_ASSERT(level == expectedLevel);

            CheckRelevLocaleDescendantsWithLevel(child, parent);
        }
    }

    Y_UNIT_TEST(CheckRelevLocaleDescendantsWithLevel) {
        for (int index = 0; index < ERelevLocale_ARRAYSIZE; ++index) {
            ERelevLocale parent = ERelevLocale(index);
            size_t level;
            UNIT_ASSERT(IsLocaleDescendantOf(parent, parent, &level));
            UNIT_ASSERT(level == 0);
            CheckRelevLocaleDescendantsWithLevel(parent, parent);
        }
    }

    Y_UNIT_TEST(CheckEULocales) {
        UNIT_ASSERT(IsLocaleDescendantOf(RL_PL, RL_EU));
        UNIT_ASSERT(IsLocaleDescendantOf(RL_FI, RL_EU));
        UNIT_ASSERT(IsLocaleDescendantOf(RL_FR, RL_EU));
        UNIT_ASSERT(IsLocaleDescendantOf(RL_BG, RL_EU));
        UNIT_ASSERT(IsLocaleDescendantOf(RL_EU, RL_UNIVERSE));
    }

    static constexpr TCateg RUSSIA = 225;
    static constexpr TCateg UKRAINE = 187;
    static constexpr TCateg BELARUS = 149;
    static constexpr TCateg KAZAKHSTAN = 159;
    static constexpr TCateg TURKEY = 983;
    static constexpr TCateg FRANCE = 124;
    static constexpr TCateg USA = 84;
    static constexpr TCateg INDONESIA = 10095;
    static constexpr TCateg GERMANY = 96;
    static constexpr TCateg UZBEKISTAN = 171;
    static constexpr TCateg DUBNA = 215;  // city in Russia
    static constexpr TCateg ODESSA = 145;  // city in Ukraine
    static constexpr TCateg BREST = 153;  // city in Belarus
    static constexpr TCateg CHIMKENT = 221;  // city in Kazakhstan
    static constexpr TCateg ADANA = 11501;  // city in Turkey
    static constexpr TCateg NANCY = 109137;  // city in France
    static constexpr TCateg DRESDEN = 10407;  // city in Germany
    static constexpr TCateg MEDAN = 10577;  // city in Indonesia
    static constexpr TCateg SAMARKAND = 10334; // city in Uzbekistan
    static constexpr TCateg FINLAND = 123;
    static constexpr TCateg HELSINKI = 10493; // city in Finland
    static constexpr TCateg POLAND = 120;
    static constexpr TCateg WARSAW = 10472; // city in Poland
    static constexpr TCateg ESTONIA = 179;
    static constexpr TCateg TALLIN = 11481; // city in Estonia
    static constexpr TCateg LATVIA = 206;
    static constexpr TCateg RIGA = 11474; // city in Latvia
    static constexpr TCateg LITHUANIA = 117;
    static constexpr TCateg VILNIUS = 11475; // city in Lithuania

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr == tld
    // tld == YST_RU
    Y_UNIT_TEST(TestGetLocale1) {
        for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
            const auto language = static_cast<ELanguage>(langIndex);

            const auto rl = GetLocale(YST_RU, language, RUSSIA);
            const auto srl = GetLocale(YST_RU, language, RUSSIA, {});
            UNIT_ASSERT_VALUES_EQUAL(RL_RU, srl.RelevLocale);
            UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
            UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
        }

        UNIT_ASSERT_VALUES_EQUAL(DUBNA, GetSearchRegion(DUBNA, RUSSIA, YST_RU, LANG_RUS, RL_RU));
    }

    struct TTriple {
        EYandexSerpTld Tld = YST_UNKNOWN;
        TCateg Country = END_CATEG;
        ERelevLocale RelevLocale = {};

        TTriple() = default;
        TTriple(const EYandexSerpTld tld, const TCateg country, const ERelevLocale relevLocale)
            : Tld{tld}
            , Country{country}
            , RelevLocale{relevLocale} {
        }
    };

    struct TQuad : public TTriple {
        ELanguage Language = LANG_UNK;

        TQuad() = default;
        TQuad(const EYandexSerpTld tld, const TCateg country, const ERelevLocale relevLocale, const ELanguage language)
                : TTriple(tld, country, relevLocale), Language(language) {
        }
    };

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr == tld
    // tld \in {YST_UA, YST_BY, YST_KZ}
    Y_UNIT_TEST(TestGetLocale2) {
        const TVector<TTriple> data = {
            {YST_UA, UKRAINE, RL_UA},
            {YST_BY, BELARUS, RL_BY},
            {YST_KZ, KAZAKHSTAN, RL_KZ},
        };

        for (const auto& example : data) {
            for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                const auto language = static_cast<ELanguage>(langIndex);

                const auto rl = GetLocale(example.Tld, language, example.Country);
                const auto srl = GetLocale(example.Tld, language, example.Country, {});
                UNIT_ASSERT_VALUES_EQUAL(example.RelevLocale, srl.RelevLocale);
                UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
            }
        }

        // language doesn't really matter here
        UNIT_ASSERT_VALUES_EQUAL(ODESSA, GetSearchRegion(ODESSA, UKRAINE, YST_UA, LANG_UKR, RL_UA));
        UNIT_ASSERT_VALUES_EQUAL(BREST, GetSearchRegion(BREST, BELARUS, YST_BY, LANG_RUS, RL_BY));
        UNIT_ASSERT_VALUES_EQUAL(CHIMKENT, GetSearchRegion(CHIMKENT, KAZAKHSTAN, YST_KZ, LANG_KAZ, RL_KZ));
        UNIT_ASSERT_VALUES_EQUAL(SAMARKAND, GetSearchRegion(SAMARKAND, UZBEKISTAN, YST_UZ, LANG_UZB, RL_UZ));
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr == tld
    // tld == YST_TR
    Y_UNIT_TEST(TestGetLocale3) {
        for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
            const auto language = static_cast<ELanguage>(langIndex);
            if (LANG_RUS == language) {
                continue;
            }

            const auto rl = GetLocale(YST_TR, language, TURKEY);
            const auto srl = GetLocale(YST_TR, language, TURKEY, {});
            UNIT_ASSERT_VALUES_EQUAL(RL_TR, srl.RelevLocale);
            UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
            UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
        }

        const auto rl = GetLocale(YST_RU, LANG_TUR, TURKEY);
        const auto srl = GetLocale(YST_RU, LANG_TUR, TURKEY, {});
        UNIT_ASSERT_VALUES_EQUAL(RL_RU, srl.RelevLocale);
        UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
        UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);

        UNIT_ASSERT_VALUES_EQUAL(ADANA, GetSearchRegion(ADANA, TURKEY, YST_TR, LANG_RUS, RL_TR));
        // relev_locale depends on language
        UNIT_ASSERT_VALUES_EQUAL(RUSSIA, GetSearchRegion(ADANA, TURKEY, YST_TR, LANG_TUR, RL_RU));
        UNIT_ASSERT_VALUES_EQUAL(RUSSIA, GetSearchRegion(ADANA, TURKEY, YST_TR, LANG_ENG, RL_RU));
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr == tld
    // tld \in {YST_FI, YST_PL}
    // {YST_EE, YST_LV, YST_LT} for spellchecker lang detector: MISSPELLDEV-304
    Y_UNIT_TEST(TestGetLocale4) {
        const TVector<TQuad> data = {
                {YST_FI, FINLAND, RL_FI, LANG_FIN},
                {YST_PL, POLAND, RL_PL, LANG_POL},
                {YST_EE, ESTONIA, RL_EE, LANG_EST},
                {YST_LV, LATVIA, RL_LV, LANG_LAV},
                {YST_LT, LITHUANIA, RL_LT, LANG_LIT}
        };
        const TVector<TCateg> regionalCities = {
                HELSINKI, WARSAW, TALLIN, RIGA, VILNIUS
        };
        for (size_t ind = 0; ind < data.size(); ind++) {
            const auto& test_set = data[ind];

            for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                const auto language = static_cast<ELanguage>(langIndex);

                const auto rl = GetLocale(test_set.Tld, language, test_set.Country);
                const auto srl = GetLocale(test_set.Tld, language, test_set.Country, {});
                UNIT_ASSERT_VALUES_EQUAL(test_set.RelevLocale, srl.RelevLocale);
                UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
            }

            const auto rl = GetLocale(YST_RU, test_set.Language, test_set.Country);
            const auto srl = GetLocale(YST_RU, test_set.Language, test_set.Country, {});
            UNIT_ASSERT_VALUES_EQUAL(RL_RU, srl.RelevLocale);
            UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
            UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);

            const auto userRegion = regionalCities[ind];
            UNIT_ASSERT_VALUES_EQUAL(userRegion,
                    GetSearchRegion(userRegion, test_set.Country, test_set.Tld, test_set.Language, test_set.RelevLocale));
            UNIT_ASSERT_VALUES_EQUAL(userRegion,
                    GetSearchRegion(userRegion, test_set.Country, test_set.Tld, LANG_ENG, test_set.RelevLocale));
            UNIT_ASSERT_VALUES_EQUAL(userRegion,
                    GetSearchRegion(userRegion, test_set.Country, test_set.Tld, LANG_RUS, test_set.RelevLocale));
        }
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr != tld и lr in (ru, ua, by, kz)
    // tld == YST_RU
    // User must be redirected.

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr != tld и lr in (ru, ua, by, kz)
    // tld \in {YST_UA, YST_BY, YST_KZ}
    // User must be redirected.

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr != tld и lr in (ru, ua, by, kz)
    // tld == YST_TR
    Y_UNIT_TEST(TestGetLocale5) {
        const TVector<TCateg> countries = {RUSSIA, UKRAINE, BELARUS, KAZAKHSTAN};
        for (const auto country : countries) {
            for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                const auto language = static_cast<ELanguage>(langIndex);
                if (LANG_RUS == language) {
                    continue;
                }

                const auto rl = GetLocale(YST_TR, language, country);
                const auto srl = GetLocale(YST_TR, language, country, {});
                UNIT_ASSERT_VALUES_EQUAL(RL_TR, srl.RelevLocale);
                UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
            }

            const auto rl = GetLocale(YST_TR, LANG_RUS, country);
            const auto srl = GetLocale(YST_TR, LANG_RUS, country, {});
            UNIT_ASSERT_VALUES_EQUAL(RL_RU, srl.RelevLocale);
            UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
            UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
        }

        // relev_locale depends on language
        UNIT_ASSERT_VALUES_EQUAL(RUSSIA, GetSearchRegion(ODESSA, UKRAINE, YST_TR, LANG_RUS, RL_RU));
        UNIT_ASSERT_VALUES_EQUAL(RUSSIA, GetSearchRegion(BREST, BELARUS, YST_TR, LANG_RUS, RL_RU));
        UNIT_ASSERT_VALUES_EQUAL(RUSSIA, GetSearchRegion(CHIMKENT, KAZAKHSTAN, YST_TR, LANG_RUS, RL_RU));

        UNIT_ASSERT_VALUES_EQUAL(TURKEY, GetSearchRegion(DUBNA, RUSSIA, YST_TR, LANG_TUR, RL_TR));
        UNIT_ASSERT_VALUES_EQUAL(TURKEY, GetSearchRegion(ODESSA, UKRAINE, YST_TR, LANG_TUR, RL_TR));
        UNIT_ASSERT_VALUES_EQUAL(TURKEY, GetSearchRegion(BREST, BELARUS, YST_TR, LANG_TUR, RL_TR));
        UNIT_ASSERT_VALUES_EQUAL(TURKEY, GetSearchRegion(CHIMKENT, KAZAKHSTAN, YST_TR, LANG_TUR, RL_TR));
        UNIT_ASSERT_VALUES_EQUAL(TURKEY, GetSearchRegion(DUBNA, RUSSIA, YST_TR, LANG_ENG, RL_TR));
        UNIT_ASSERT_VALUES_EQUAL(TURKEY, GetSearchRegion(ODESSA, UKRAINE, YST_TR, LANG_ENG, RL_TR));
        UNIT_ASSERT_VALUES_EQUAL(TURKEY, GetSearchRegion(BREST, BELARUS, YST_TR, LANG_ENG, RL_TR));
        UNIT_ASSERT_VALUES_EQUAL(TURKEY, GetSearchRegion(CHIMKENT, KAZAKHSTAN, YST_TR, LANG_ENG, RL_TR));

        UNIT_ASSERT_VALUES_EQUAL(UZBEKISTAN, GetSearchRegion(CHIMKENT, KAZAKHSTAN, YST_TR, LANG_ENG, RL_UZ));
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr != tld
    // tld \in {YST_FI, YST_PL}
    // {YST_EE, YST_LV, YST_LT} for spellchecker lang detector: MISSPELLDEV-304
    Y_UNIT_TEST(TestGetLocale6) {
        const TVector<TCateg> countries = {RUSSIA, UKRAINE, BELARUS, KAZAKHSTAN};
        const TVector<TCateg> cities = {DUBNA, ODESSA, BREST, CHIMKENT};

        const TVector<TQuad> data = {
                {YST_FI, FINLAND, RL_FI, LANG_FIN},
                {YST_PL, POLAND, RL_PL, LANG_POL},
                {YST_EE, ESTONIA, RL_EE, LANG_EST},
                {YST_LV, LATVIA, RL_LV, LANG_LAV},
                {YST_LT, LITHUANIA, RL_LT, LANG_LIT}
        };
        for (const auto test_set : data) {
            for (size_t ind = 0; ind < countries.size(); ind++) {
                const auto country = countries[ind];
                const auto city = cities[ind];

                for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                    const auto language = static_cast<ELanguage>(langIndex);

                    const auto rl = GetLocale(test_set.Tld, language, country);
                    const auto srl = GetLocale(test_set.Tld, language, country, {});
                    UNIT_ASSERT_VALUES_EQUAL(test_set.RelevLocale, srl.RelevLocale);
                    UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                    UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
                }

                // relev_locale depends on language
                UNIT_ASSERT_VALUES_EQUAL(test_set.Country,
                                         GetSearchRegion(city, country, test_set.Tld, LANG_RUS, test_set.RelevLocale));
                UNIT_ASSERT_VALUES_EQUAL(test_set.Country,
                                         GetSearchRegion(city, country, test_set.Tld, test_set.Language, test_set.RelevLocale));
                UNIT_ASSERT_VALUES_EQUAL(test_set.Country,
                                         GetSearchRegion(city, country, test_set.Tld, LANG_ENG, test_set.RelevLocale));
            }
        }
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr != tld and lr not in (ru, ua, by, kz)
    // tld == YST_RU
    Y_UNIT_TEST(TestGetLocale7) {
        // some random countries from geobase
        const TVector<TCateg> countries = {10090, 10054, 20826, 10088, 21182,
                                           20856, 93, 168, 21536, 211, 113, 167};
        for (const auto country : countries) {
            for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                const auto language = static_cast<ELanguage>(langIndex);

                const auto rl = GetLocale(YST_RU, language, country);
                const auto srl = GetLocale(YST_RU, language, country, {});
                UNIT_ASSERT_VALUES_EQUAL(RL_RU, srl.RelevLocale);
                UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
            }
        }

        // language doesn't matter here
        UNIT_ASSERT_VALUES_EQUAL(RUSSIA, GetSearchRegion(NANCY, FRANCE, YST_RU, LANG_RUS, RL_RU));
        UNIT_ASSERT_VALUES_EQUAL(RUSSIA, GetSearchRegion(NANCY, FRANCE, YST_RU, LANG_FRE, RL_RU));
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr != tld and lr not in (ru, ua, by, kz)
    // tld \in {YST_UA, YST_BY, YST_KZ}
    Y_UNIT_TEST(TestGetLocale8) {
        // some random countries from geobase
        const TVector<TCateg> countries = {10090, 10054, 20826, 10088, 21182,
                                           20856, 93, 168, 21536, 211, 113, 167};
        const TVector<std::pair<EYandexSerpTld, ERelevLocale>> data =
            {{YST_UA, RL_UA}, {YST_BY, RL_BY}, {YST_KZ, RL_KZ}};

        for (const auto& example : data) {
            for (const auto country : countries) {
                for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                    const auto language = static_cast<ELanguage>(langIndex);

                    const auto rl = GetLocale(example.first, language, country);
                    const auto srl = GetLocale(example.first, language, country, {});
                    UNIT_ASSERT_VALUES_EQUAL(example.second, srl.RelevLocale);
                    UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                    UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
                }
            }
        }
        // language doesn't matter here
        UNIT_ASSERT_VALUES_EQUAL(UKRAINE, GetSearchRegion(NANCY, FRANCE, YST_UA, LANG_UKR, RL_UA));
        UNIT_ASSERT_VALUES_EQUAL(BELARUS, GetSearchRegion(NANCY, FRANCE, YST_BY, LANG_BEL, RL_BY));
        UNIT_ASSERT_VALUES_EQUAL(KAZAKHSTAN, GetSearchRegion(NANCY, FRANCE, YST_KZ, LANG_KAZ, RL_KZ));
        UNIT_ASSERT_VALUES_EQUAL(UKRAINE, GetSearchRegion(NANCY, FRANCE, YST_UA, LANG_ENG, RL_UA));
        UNIT_ASSERT_VALUES_EQUAL(BELARUS, GetSearchRegion(NANCY, FRANCE, YST_BY, LANG_ENG, RL_BY));
        UNIT_ASSERT_VALUES_EQUAL(KAZAKHSTAN, GetSearchRegion(NANCY, FRANCE, YST_KZ, LANG_ENG, RL_KZ));
        UNIT_ASSERT_VALUES_EQUAL(UZBEKISTAN, GetSearchRegion(NANCY, FRANCE, YST_KZ, LANG_ENG, RL_UZ));
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // lr != tld and lr not in (ru, ua, by, kz)
    // tld == YST_TR
    Y_UNIT_TEST(TestGetLocale9) {
        // some random countries from geobase
        const TVector<TCateg> countries = {10090, 10054, 20826, 10088, 21182,
                                           20856, 93, 168, 21536, 211, 113, 167};
        for (const auto country : countries) {
            for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                const auto language = static_cast<ELanguage>(langIndex);
                if (LANG_RUS == language) {
                    continue;
                }

                const auto rl = GetLocale(YST_TR, language, country);
                const auto srl = GetLocale(YST_TR, language, country, {});
                UNIT_ASSERT_VALUES_EQUAL(RL_TR, srl.RelevLocale);
                UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
            }

            const auto rl = GetLocale(YST_TR, LANG_RUS, country);
            const auto srl = GetLocale(YST_TR, LANG_RUS, country, {});
            UNIT_ASSERT_VALUES_EQUAL(RL_RU, srl.RelevLocale);
            UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
            UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
        }

        UNIT_ASSERT_VALUES_EQUAL(TURKEY, GetSearchRegion(NANCY, FRANCE, YST_TR, LANG_TUR, RL_TR));
        // language doesn't matter here, until it's turkish
        UNIT_ASSERT_VALUES_EQUAL(RUSSIA, GetSearchRegion(NANCY, FRANCE, YST_TR, LANG_RUS, RL_RU));
        UNIT_ASSERT_VALUES_EQUAL(RUSSIA, GetSearchRegion(NANCY, FRANCE, YST_TR, LANG_ENG, RL_RU));
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // tld == YST_COM
    // qmpl \in {ru, ua, by, kz, tr}
    Y_UNIT_TEST(TestGetLocale10) {
        // some random countries from geobase
        const TVector<TCateg> countries = {10090, 10054, 20826, 10088, 21182,
                                           20856, 93, 168, 21536, 211, 113, 167};
        const TVector<std::pair<ELanguage, ERelevLocale>> data =
            {{LANG_RUS, RL_RU}, {LANG_UKR, RL_UA}, {LANG_BEL, RL_BY},
             {LANG_KAZ, RL_KZ}, {LANG_TUR, RL_TR}};
        for (const auto country : countries) {
            for (const auto& example : data) {
                const auto rl = GetLocale(YST_COM, example.first, country);
                const auto srl = GetLocale(YST_COM, example.first, country, {});
                UNIT_ASSERT_VALUES_EQUAL(example.second, srl.RelevLocale);
                UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
            }
        }
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // tld == YST_COM
    // qmpl \in {ru, ua, by, kz, tr}
    Y_UNIT_TEST(TestGetLocale8GetSearchRegion) {
        const TVector<TCateg> countries = {RUSSIA, UKRAINE, BELARUS, KAZAKHSTAN, TURKEY};
        const TVector<TCateg> cities = {DUBNA, ODESSA, BREST, CHIMKENT, ADANA};
        const TVector<ERelevLocale> rl = {RL_RU, RL_UA, RL_BY, RL_KZ, RL_TR};
        const TVector<ELanguage> lang = {LANG_RUS, LANG_UKR, LANG_BEL, LANG_KAZ, LANG_TUR};
        Y_ENSURE(countries.size() == cities.size(), "Wrong size");
        Y_ENSURE(cities.size() == rl.size(), "Wrong size");
        for (const auto index : xrange(countries.size())) {
            for (const auto jindex : xrange(rl.size())) {
                const auto searchRegion = GetSearchRegion(cities[index], countries[index], YST_COM,
                                                          lang[jindex], rl[jindex]);
                if (index == jindex) {
                    UNIT_ASSERT_VALUES_EQUAL(cities[index], searchRegion);
                } else {
                    UNIT_ASSERT_VALUES_EQUAL(countries[jindex], searchRegion);
                }
            }
        }
    }


    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // tld == YST_COM
    // qmpl \not \in {ru, ua, by, kz, tr}
    // no SPOK countries provided
    Y_UNIT_TEST(TestGetLocale11) {
        // some random countries from geobase
        const TVector<TCateg> countries = {10090, 10054, 20826, 10088, 21182,
                                           20856, 93, 168, 21536, 211, 113, 167};
        const THashSet<ELanguage> languageExceptions = {LANG_RUS, LANG_UKR, LANG_BEL, LANG_KAZ, LANG_TUR, LANG_UZB};
        for (const auto country : countries) {
            for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                const auto language = static_cast<ELanguage>(langIndex);
                if (languageExceptions.contains(language)) {
                    continue;
                }

                const auto rl = GetLocale(YST_COM, language, country);
                const auto srl = GetLocale(YST_COM, language, country, {});
                UNIT_ASSERT_VALUES_EQUAL(RL_WORLD, srl.RelevLocale);
                UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(USA, GetSearchRegion(DUBNA, RUSSIA, YST_COM, LANG_ENG, RL_WORLD));
        UNIT_ASSERT_VALUES_EQUAL(USA, GetSearchRegion(ODESSA, UKRAINE, YST_COM, LANG_ENG, RL_WORLD));
        UNIT_ASSERT_VALUES_EQUAL(USA, GetSearchRegion(BREST, BELARUS, YST_COM, LANG_ENG, RL_WORLD));
        UNIT_ASSERT_VALUES_EQUAL(USA, GetSearchRegion(CHIMKENT, KAZAKHSTAN, YST_COM, LANG_ENG, RL_WORLD));
        UNIT_ASSERT_VALUES_EQUAL(USA, GetSearchRegion(ADANA, TURKEY, YST_COM, LANG_ENG, RL_WORLD));
        UNIT_ASSERT_VALUES_EQUAL(USA, GetSearchRegion(NANCY, FRANCE, YST_COM, LANG_ENG, RL_WORLD));
        UNIT_ASSERT_VALUES_EQUAL(USA, GetSearchRegion(SAMARKAND, UZBEKISTAN, YST_UZ, LANG_UZB, RL_WORLD));
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // test for SPOK countries
    Y_UNIT_TEST(TestGetLocale12) {
        const THashSet<TCateg> spokCountries = {INDONESIA, GERMANY};
        const THashSet<ELanguage> languageExceptions = {LANG_RUS, LANG_UKR, LANG_BEL,
                                                         LANG_KAZ, LANG_TUR, LANG_UZB};
        const TVector<TCateg> countries = {INDONESIA, GERMANY, FRANCE};
        // the last one (RL_UNIVERCE) here is fake, it provided only for the sake of consistency
        const TVector<ERelevLocale> relevLocales = {RL_ID, RL_DE, RL_UNIVERSE};
        Y_ENSURE(countries.size() == relevLocales.size(), "Wrong size");
        for (const auto index : xrange(countries.size())) {
            for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                const auto language = static_cast<ELanguage>(langIndex);
                if (languageExceptions.contains(language)) {
                    continue;
                }

                const auto srl = GetLocale(YST_COM, language, countries[index], spokCountries);
                const bool flagRow5 = spokCountries.contains(countries[index]);
                const bool flagRow6 = Lang2SPOKCountry(language)
                                        && spokCountries.contains(Lang2SPOKCountry(language)->CountryId);
                if (flagRow5) {
                    UNIT_ASSERT_VALUES_EQUAL(relevLocales[index], srl.RelevLocale);
                    UNIT_ASSERT_VALUES_EQUAL(true, srl.IsSpokCountry);
                } else if (flagRow6) {
                    UNIT_ASSERT_VALUES_EQUAL(Lang2SPOKCountry(language)->RelevLocale, srl.RelevLocale);
                    UNIT_ASSERT_VALUES_EQUAL(true, srl.IsSpokCountry);
                } else {
                    UNIT_ASSERT_VALUES_EQUAL(RL_WORLD, srl.RelevLocale);
                    UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                }
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(DRESDEN, GetSearchRegion(DRESDEN, GERMANY, YST_COM, LANG_ENG, RL_DE));
        UNIT_ASSERT_VALUES_EQUAL(MEDAN, GetSearchRegion(MEDAN, INDONESIA, YST_COM, LANG_ENG, RL_ID));
        UNIT_ASSERT_VALUES_EQUAL(GERMANY, GetSearchRegion(MEDAN, INDONESIA, YST_COM, LANG_ENG, RL_DE));
        UNIT_ASSERT_VALUES_EQUAL(INDONESIA, GetSearchRegion(DRESDEN, GERMANY, YST_COM, LANG_ENG, RL_ID));
    }

    // https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
    // tld == YST_EU
    // qmpl doesn't matter
    // lr \in EU_Countries -> search_region = lr
    // lr \not \in EU_Countries -> search_region = USA && relev_locale = RL_WORLD
    Y_UNIT_TEST(TestGetLocale13) {
        const TVector<TCateg> countries = {FRANCE, FINLAND, POLAND};
        const TVector<TCateg> cities = {NANCY, HELSINKI, WARSAW};
        const TVector<NRl::ERelevLocale> locales = {RL_FR, RL_FI, RL_PL};
        const TVector<ELanguage> languages = {LANG_FRE, LANG_FIN, LANG_POL};

        for (size_t ind = 0; ind < countries.size(); ind++) {
            const auto country = countries[ind];
            const auto city = cities[ind];

            for (int langIndex = 0; langIndex < static_cast<int>(LANG_MAX); ++langIndex) {
                const auto language = static_cast<ELanguage>(langIndex);

                const auto rl = GetLocale(YST_EU, language, country);
                const auto srl = GetLocale(YST_EU, language, country, {});
                UNIT_ASSERT_VALUES_EQUAL(locales[ind], srl.RelevLocale);
                UNIT_ASSERT_VALUES_EQUAL(false, srl.IsSpokCountry);
                UNIT_ASSERT_VALUES_EQUAL(rl, srl.RelevLocale);
            }

            // relev_locale depends on language
            UNIT_ASSERT_VALUES_EQUAL(city,
                                     GetSearchRegion(city, country, YST_EU, LANG_RUS, locales[ind]));
            UNIT_ASSERT_VALUES_EQUAL(city,
                                     GetSearchRegion(city, country, YST_EU, languages[ind], locales[ind]));
            UNIT_ASSERT_VALUES_EQUAL(city,
                                     GetSearchRegion(city, country, YST_EU, LANG_ENG, locales[ind]));

        }
        UNIT_ASSERT_VALUES_EQUAL(RL_WORLD, GetLocale(YST_EU, LANG_RUS, RUSSIA));
        UNIT_ASSERT_VALUES_EQUAL(USA,
                                 GetSearchRegion(BREST, RUSSIA, YST_EU, LANG_RUS, RL_WORLD));
    }

}
