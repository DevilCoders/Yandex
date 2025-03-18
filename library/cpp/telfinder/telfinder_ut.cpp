#include "text_telfinder.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/stream/str.h>
#include <util/stream/output.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

TString PhonesToString(const TFoundPhones& p) {
    TString res;
    if (p.empty()) {
        return res;
    }
    res = p.front().Phone.ToPhoneWithCountry();
    for (size_t i = 1; i < p.size(); ++i) {
        res.append(' ').append(p[i].Phone.ToPhoneWithCountry());
    }

    return res;
}

TFoundPhones ParsePhones(const TString& text, TTelFinder* telFinder = nullptr) {
    THolder<TTelFinder> telFinderHolder;
    if (nullptr == telFinder) {
        telFinderHolder.Reset(new TTelFinder());
        telFinder = telFinderHolder.Get();
    }

    TTextTelProcessor finder(telFinder);
    finder.ProcessText(UTF8ToWide(text));

    TFoundPhones phones;
    finder.GetFoundPhones(phones);
    return phones;
}

TString TestIncludedSchemes(const TString& text) {
    TFoundPhones p = ParsePhones(text);
    if (p.empty())
        return TString();

    return p.front().Phone.ToPhoneWithCountry();
}

TString TestLoadedSchemes(const TString& text) {
    THashMap<TString, TAreaScheme> schemesSet;
    schemesSet.insert(std::make_pair(TString("+####(####)###-##-##"), TAreaScheme(0, 4, 0, 4, false)));

    TPhoneSchemes schemes(schemesSet);
    THolder<TTelFinder> telFinder(new TTelFinder(schemes));

    TFoundPhones p = ParsePhones(text, telFinder.Get());
    if (p.empty())
        return TString();

    return p.front().Phone.ToPhoneWithCountry();
}

using __TPair = std::pair<size_t, size_t>;
Y_DECLARE_OUT_SPEC(inline, __TPair, out, p) {
    out << '{' << p.first << ',' << p.second << '}';
}

TString TestLocation(const TString& text) {
    TFoundPhones p = ParsePhones(text);
    if (p.empty())
        return TString();

    TStringStream str;
    str << p.front().Location.PhonePos
        << ' ' << p.front().Location.CountryCodePos
        << ' ' << p.front().Location.AreaCodePos
        << ' ' << p.front().Location.LocalPhonePos;
    return str.Str();
}

#define CHECK_LOCATION(phone, location) \
    UNIT_ASSERT_EQUAL_C(TestLocation(phone), location, ", actual = " << TestLocation(phone));

Y_UNIT_TEST_SUITE(TestTelfinder) {
    Y_UNIT_TEST(PhoneParseTest) {
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("940-29-75 номер телефона")), "9402975");
    }

    Y_UNIT_TEST(LongestPhoneTest) {
        THashMap<TString, TAreaScheme> schemesSet;
        schemesSet.insert(std::make_pair(TString("_###_##_##"), TAreaScheme(0, 0, 0, 0, false)));
        schemesSet.insert(std::make_pair(TString("_###_##"), TAreaScheme(0, 0, 0, 0, false)));
        schemesSet.insert(std::make_pair(TString("_###-###_##_##"), TAreaScheme(0, 0, 0, 3, false)));
        schemesSet.insert(std::make_pair(TString("_###-###"), TAreaScheme(0, 0, 0, 0, false)));

        TPhoneSchemes schemes(schemesSet);
        THolder<TTelFinder> telFinder(new TTelFinder(schemes));

        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("995 09 09 отзывы", telFinder.Get())), "9950909");
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("отзывы 460-470 60 70", telFinder.Get())), "(460)4706070");
    }

    Y_UNIT_TEST(IncludedSchemesTest) {
        //UNIT_ASSERT_EQUAL(TestIncludedSchemes("..+1(222)333-44-55.."), "");                       //it is not working now.
        UNIT_ASSERT_EQUAL(TestIncludedSchemes(".. +1(222)333-44-55 .."), "1(222)3334455");
        UNIT_ASSERT_EQUAL(TestIncludedSchemes("+1 (222) 333 - 44 - 55 .."), "1(222)3334455");
        UNIT_ASSERT_EQUAL(TestIncludedSchemes("..+1 ( 222 ) 333 - 44 - 55"), "1(222)3334455");
        //UNIT_ASSERT_EQUAL(TestIncludedSchemes("1(222)333-44-55 .."), "");                         //it is not working now.
        UNIT_ASSERT_EQUAL(TestIncludedSchemes(" +1111(222222)333-44-55 "), "");
        UNIT_ASSERT_EQUAL(TestIncludedSchemes("+1 ( 222 ) 3 33 "), "");
        UNIT_ASSERT_EQUAL(TestIncludedSchemes("+38 (050) 767-85-56"), "380(50)7678556");
        UNIT_ASSERT_EQUAL(TestIncludedSchemes("+49-0482-35-53-55"), "49(0482)355355");
        UNIT_ASSERT_EQUAL(TestIncludedSchemes("+96614677440"), "966(1)4677440");
    }

    Y_UNIT_TEST(LoadedSchemesTest) {
        UNIT_ASSERT_EQUAL(TestLoadedSchemes(".. +1(222)333-44-55 .."), "");
        UNIT_ASSERT_EQUAL(TestLoadedSchemes("+1111(2222) 333- 44- 55"), "1111(2222)3334455");
    }

    Y_UNIT_TEST(Location) {
        CHECK_LOCATION("+74997503030", "{1,2} {1,2} {1,2} {1,2}");
        CHECK_LOCATION("(499)7503030", "{1,3} {0,0} {1,2} {2,3}");
        CHECK_LOCATION("тел: (8442) 93-11-30", "{1,5} {0,0} {1,2} {2,5}");
        CHECK_LOCATION("тел: +00(257)235 11-111", "{2,6} {2,3} {3,4} {4,6}");
        CHECK_LOCATION("123 1 1111111", "{0,3} {0,1} {1,2} {2,3}");
    }

    Y_UNIT_TEST(SplitMultiplePhones) {
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("+7(495)122-11-12,+7(495)122-11-12")), "7(495)1221112 7(495)1221112");
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("+7(495)122-11-12 +7(495)122-11-12")), "7(495)1221112 7(495)1221112");

        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("(495)122-11-12,(495)122-11-12")), "(495)1221112 (495)1221112");
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("(495)122-11-12 (495)122-11-12")), "(495)1221112 (495)1221112");

        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("7(495)122-11-12,7(495)122-11-12")), "7(495)1221112 7(495)1221112");
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("7(495)122-11-12 7(495)122-11-12")), "7(495)1221112 7(495)1221112");

        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("(812) 348 09 54 89117390748")), "(812)3480954");
        // Negative test for mutiple digits
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("345 940 29 75 351")), "");
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("940 29 75 351")), "");
    }

    Y_UNIT_TEST(SchemesWithDigits) {
        THashMap<TString, TAreaScheme> schemesSet;
        schemesSet.insert(std::make_pair(TString("_495_###_##_##"), TAreaScheme(0, 0, 0, 3, false)));
        schemesSet.insert(std::make_pair(TString("_91#_###_##_##"), TAreaScheme(0, 0, 0, 3, false)));
        schemesSet.insert(std::make_pair(TString("_###_##_00"), TAreaScheme(0, 0, 0, 0, false)));

        TPhoneSchemes schemes(schemesSet);
        THolder<TTelFinder> telFinder(new TTelFinder(schemes));

        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("495 888 88 88", telFinder.Get())), "(495)8888888");
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("496 888 88 88", telFinder.Get())), "");

        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("916 888 88 88", telFinder.Get())), "(916)8888888");
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("919 888 88 88", telFinder.Get())), "(919)8888888");
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("926 888 88 88", telFinder.Get())), "");

        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("900 54 00", telFinder.Get())), "9005400");
        UNIT_ASSERT_EQUAL(PhonesToString(ParsePhones("900 54 01", telFinder.Get())), "");
    }
}
