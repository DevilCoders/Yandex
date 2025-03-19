#include <kernel/decapitalizer/decapitalizer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.cpp>
#include <util/charset/wide.h>


Y_UNIT_TEST_SUITE(TDecapitalizerUnitTests) {

    void TestUniqWtroka(const TVector<std::pair<TString, TString>>& tests, const THashSet<TUtf16String>& abbrevs, const TVector<TUtf16String>& hints,
                        ELanguage mainLang, const TLangMask& mask) {
        NDecapitalizer::TDecapitalizer decapitalizer(abbrevs, hints, mainLang, mask);
        for (const auto& test : tests) {
            TUtf16String text = UTF8ToWide(test.first);
            decapitalizer.Decapitalize(text);
            UNIT_ASSERT_EQUAL(text, UTF8ToWide(test.second));
        }
    }

    void TestSharedWtroka(const TVector<std::pair<TString, TString>>& tests, const THashSet<TUtf16String>& abbrevs, const TVector<TUtf16String>& hints,
                          ELanguage mainLang, const TLangMask& mask) {
        NDecapitalizer::TDecapitalizer decapitalizer(abbrevs, hints, mainLang, mask);
        for (const auto& test : tests) {
            TUtf16String text = UTF8ToWide(test.first);
            TUtf16String textCopy = text;
            decapitalizer.Decapitalize(text);
            UNIT_ASSERT_EQUAL(text, UTF8ToWide(test.second));
            UNIT_ASSERT_EQUAL(textCopy, UTF8ToWide(test.first));
        }
    }

    void Test(const TVector<std::pair<TString, TString>>& tests, const THashSet<TUtf16String>& abbrevs, const TVector<TUtf16String>& hints,
              ELanguage mainLang = NDecapitalizer::DEFAULT_MAIN_LANGUAGE, const TLangMask& mask = NDecapitalizer::DEFAULT_LANG_MASK) {
        TestUniqWtroka(tests, abbrevs, hints, mainLang, mask);
        TestSharedWtroka(tests, abbrevs, hints, mainLang, mask);
    }

    void CerrTest(const TVector<std::pair<TString, TString>>& tests, const THashSet<TUtf16String>& abbrevs, const TVector<TUtf16String>& hints,
                  ELanguage mainLang = NDecapitalizer::DEFAULT_MAIN_LANGUAGE, const TLangMask& mask = NDecapitalizer::DEFAULT_LANG_MASK) {
        NDecapitalizer::TDecapitalizer decapitalizer(abbrevs, hints, mainLang, mask);
        for (const auto& test : tests) {
            TUtf16String text = UTF8ToWide(test.first);
            decapitalizer.Decapitalize(text);
            if (text != UTF8ToWide(test.second)) {
                Cerr << "Expected: " << test.second << Endl;
                Cerr << "Found:    " << text << Endl;
            }
        }
    }

    Y_UNIT_TEST(TestAbbrevs) {
        TVector<std::pair<TString, TString>> tests = {
            {
                "TTL—АББРЕВИАТУРА. МОЖЕТ ОЗНАЧАТЬ: Time to live — время жизни пакета данных в протоколе IP, время актуальности записей DNS.",
                "TTL—аббревиатура. Может означать: Time to live — время жизни пакета данных в протоколе IP, время актуальности записей DNS."
            },
            {
                "Examples of ROMAN NUMBERS: XX, VX, IX, XL",
                "Examples of roman numbers: XX, VX, IX, XL"
            },
            {
                "СЛОВА С ЦИФРАМИ и маленькими буквами: IPv6, RFC3513, iPHONE, IpHONE",
                "Слова с цифрами и маленькими буквами: IPv6, RFC3513, iPHONE, IpHONE"
            },
            {
                "ЗДЕСЬ есть одиночные заглавные буквы: N, Ф; и инициалы: А.С. Пушкин",
                "Здесь есть одиночные заглавные буквы: N, Ф; и инициалы: А.С. Пушкин",
            }
        };

        THashSet<TUtf16String> abbrevs = {
            u"TTL",
            u"IP",
            u"DNS",
        };
        TVector<TUtf16String> hints = {};
        Test(tests, abbrevs, hints);
    }


    Y_UNIT_TEST(TestHints) {
        TVector<std::pair<TString, TString>> tests = {
            {
                "СВЕТЛАНА. ИСТОРИЯ И ЗНАЧЕНИЕ ИМЕНИ СВЕТЛАНА - Светлана (светлая, чистая) - имя славянского происхождения - от слова \"светлая\".",
                "Светлана. История и значение имени Светлана - Светлана (светлая, чистая) - имя славянского происхождения - от слова \"светлая\"."
            },
            {
                "PSTN (Public switched telephone network; ТФОП, ТЕЛЕФОННАЯ сеть общего пользования) — всемирная телефонная сеть, к которой подключены факсы, модемы, АТС.",
                "PSTN (Public switched telephone network; ТФОП, телефонная сеть общего пользования) — всемирная телефонная сеть, к которой подключены факсы, модемы, АТС."
            },
            {
                "ПЕРВОЕ СЛОВО ОБЫЧНОЕ И ЕСТЬ В ХИНТАХ.",
                "Первое слово обычное и есть в хинтах.",
            }
        };
        THashSet<TUtf16String> abbrevs = {
            u"ТФОП",
            u"АТС"
        };
        TVector<TUtf16String> hints = {
            u"PSTN, Public Switched Telephone Network) — это сеть, для доступа к которой используются обычные телефонные аппараты.",
            u"Абоненты подключаются к PSTN-сети (Public Switched Telephone Network) — телефонной сети общего пользования, она же ТСОП, ТфОП.",
            u"Смотреть что такое pstn в других словарях",
            u"У Светланы много друзей.",
            u"Первый первая первый"
        };
        Test(tests, abbrevs, hints);
    }

    Y_UNIT_TEST(TestLangs) {
        TVector<std::pair<TString, TString>> testsTur = {
            {
                "YABANCI",
                "Yabancı"
            }
        };
        Test(testsTur, {}, {}, LANG_TUR, TLangMask(LANG_TUR));
        TVector<std::pair<TString, TString>> testsEng = {
            {
                "ENGLISH",
                "English"
            }
        };
        Test(testsEng, {}, {}, LANG_ENG, TLangMask(LANG_ENG));
    }
}
