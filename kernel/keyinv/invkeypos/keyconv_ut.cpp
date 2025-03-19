#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/string.h>
#include <library/cpp/charset/doccodes.h>
#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>

#include "keyconv.h"
#include "keychars.h"

namespace {

    const unsigned char STROKA[] = { 0xf1, 0xf2, 0xf0, 0xee, 0xea, 0xe0, 0x00 }; // yandex encoded 'TCiString' in Russian

}

class TFormToKeyConvertorTest: public TTestBase
{
    UNIT_TEST_SUITE(TFormToKeyConvertorTest);
        UNIT_TEST(testConvert);
        UNIT_TEST(testExtSymbols);
        UNIT_TEST(testConvertUTF8);
        UNIT_TEST(testConvertUnicodeToYandex);
        UNIT_TEST(testConvertAttrValue);
        UNIT_TEST(testConvertFromYandexToUTF8);
    UNIT_TEST_SUITE_END();
public:
    void testConvert();
    void testExtSymbols();
    void testConvertUnicodeToYandex();
    void testConvertUTF8();
    void testConvertAttrValue();
    void testConvertFromYandexToUTF8();
};

void TFormToKeyConvertorTest::testConvert() {
    TFormToKeyConvertor convertor;
    wchar16 str[] = {0x0441, 0x0442, 0x0440, 0x043e, 0x043a, 0x0430};
    UNIT_ASSERT(!HasExtSymbols(str,6));
    const char* res = convertor.Convert(str, sizeof(str)/sizeof(wchar16));
    UNIT_ASSERT_VALUES_EQUAL(TString((const char*)STROKA), res);

    str[1] = 0x0165; // #LATIN SMALL LETTER T WITH CARON
    UNIT_ASSERT(HasExtSymbols(str,6));
    res = convertor.Convert(str, sizeof(str)/sizeof(wchar16));
    ui8 q[] = {0x7f, 0xd1, 0x81, 0xc5, 0xa5, 0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xba, 0xd0, 0xb0, 0x00};
    UNIT_ASSERT(memcmp(q, res, sizeof(q)) == 0);
    TString yres = RecodeToYandex(CODES_UTF8, res + 1);
    // EncodeTo[CODES_YANDEX] removes daicritics - transforms 'latin small letter T with caron' into 't'
    {
        TString s((const char*)STROKA);
        s.replace(1, 1, 1, 't');
        UNIT_ASSERT_VALUES_EQUAL(yres, s);
    }

    str[1] = 0x039b; // \Lambda
    UNIT_ASSERT(HasExtSymbols(str,6));
    res = convertor.Convert(str, sizeof(str)/sizeof(wchar16));
    q[3] = 0xce;
    q[4] = 0x9b;
    UNIT_ASSERT(memcmp(q, res, sizeof(q)) == 0);
    yres = RecodeToYandex(CODES_UTF8, res + 1);
    {
        // 0xA6 #CYRILLIC CAPITAL LETTER IOTIFIED BIG YUS, default char for all capital letters in csYandex
        TString s((const char*)STROKA);
        s.replace(1, 1, 1, '\246');
        UNIT_ASSERT_VALUES_EQUAL(yres, s);
    }
}

void TFormToKeyConvertorTest::testConvertUTF8() {
    TFormToKeyConvertor convertor;
    unsigned char str1[] = {0xd1, 0x81, 0xd1, 0x82, 0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xba, 0xd0, 0xb0}; // utf8 encoded 'TCiString' in Russian
    const char* res = convertor.ConvertUTF8((const char*)str1, sizeof(str1));
    UNIT_ASSERT_VALUES_EQUAL(TString((const char*)STROKA), res);

    unsigned char str2[] = {0xd1, 0x81, 0xc5, 0xa5, 0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xba, 0xd0, 0xb0, 0x00};
    res = convertor.ConvertUTF8((const char*)str2, sizeof(str2)-1);
    UNIT_ASSERT_EQUAL(res[0],UTF8_FIRST_CHAR);
    UNIT_ASSERT(memcmp(str2, res+1, sizeof(str2)) == 0);
}

void TFormToKeyConvertorTest::testConvertAttrValue() {
    TFormToKeyConvertor convertor;
    size_t written = 0;

    TUtf16String w = u"Dear People of Astana"; // in English
    convertor.ConvertAttrValue(w.c_str(), w.size(), written);
    UNIT_ASSERT_VALUES_EQUAL(written, 21u);

    w = u"Дорогие астанчане!"; // in Russian
    convertor.ConvertAttrValue(w.c_str(), w.size(), written);
    UNIT_ASSERT_VALUES_EQUAL(written, 18u);

    w = u"Құрметті астаналықтар!"; // in Kazakh
    const char* res = convertor.ConvertAttrValue(w.c_str(), w.size(), written);
    UNIT_ASSERT(res[0] == 0x7F);
    UNIT_ASSERT_VALUES_EQUAL(written, 43u);

    w = u"Дорогие астаналықтар!"; // mixed langugages
    res = convertor.ConvertAttrValue(w.c_str(), w.size(), written);
    UNIT_ASSERT(res[0] != 0x7F && res[16] == 0x7F);
    UNIT_ASSERT_VALUES_EQUAL(written, 26u);

    const wchar16 word[] = {0x0441, 0x0000, 0x0441, 0x0441, 0x0441, 0x0441};
    res = convertor.ConvertAttrValue(word, 6u, written);
    UNIT_ASSERT_VALUES_EQUAL(written, 11u);
}

void TFormToKeyConvertorTest::testConvertFromYandexToUTF8() {
    const char englishText[] = "Test"; // in English
    TString s;
    ConvertFromYandexToUTF8(englishText, s);
    UNIT_ASSERT_EQUAL(s, TString(englishText));

    const char yandexText[] = "\xcf\xf0\xee\xe2\xe5\xf0\xea\xe0"; // in CP-1251
    ConvertFromYandexToUTF8(yandexText, s);
    UNIT_ASSERT_EQUAL(s, TString("Проверка"));

    const char invalidUTFText[] = "\x7f\xcf\xf0\xee\xe2\xe5\xf0\xea\xe0"; // in CP-1251
    ConvertFromYandexToUTF8(invalidUTFText, s);
    UNIT_ASSERT_EQUAL(s, TString("\x7fПроверка"));

    const char validUTFText[] = "\x7fПроверка"; // in UTF-8
    ConvertFromYandexToUTF8(validUTFText, s);
    UNIT_ASSERT_EQUAL(s, TString("Проверка"));

    s = "Test";
    const char emptyText[] = "";
    ConvertFromYandexToUTF8(emptyText, s);
    UNIT_ASSERT_EQUAL(s, TString(""));
}

void TFormToKeyConvertorTest::testExtSymbols() {
    TUtf16String w = u"Съешь ещё этих мягких французских булок, да выпей же чаю.";
    UNIT_ASSERT(!HasExtSymbols(w.c_str(), w.size()));

    const wchar16 word[] = {0x0441, 0x0442, 0x0000, 0x0441, 0x0000};
    UNIT_ASSERT(HasExtSymbols(word, 5u));

    const wchar16 word1[] = {0x0441, 0x0441, 0x0441, 0x0165};
    UNIT_ASSERT(HasExtSymbols(word1, 4u));
}

void TFormToKeyConvertorTest::testConvertUnicodeToYandex() {
    // char* buffer, size_t buflen, const wchar16* text, size_t textlen, size_t& written
    size_t written = 0;
    TString buf = "Ya crevedko";
    const wchar16 word[] = {0x0441, 0x0441, 0x0000, 0x0441, 0x0000};
    ConvertUnicodeToYandex(buf.Detach(), 11u, word, 5u, written);
    UNIT_ASSERT_EQUAL(written, 0u);
    UNIT_ASSERT_EQUAL(buf[0], -15);

    const wchar16 anotherOne[] = {0xc3, 0x28};
    ConvertUnicodeToYandex(buf.Detach(), 11u, anotherOne, 2u, written);
    UNIT_ASSERT_EQUAL(written, 0u);
    UNIT_ASSERT_EQUAL(buf[0], 0);

    const wchar16 goodWord[] = {0x0441, 0x0441, 0x0441, 0x0441, 0x0441};
    ConvertUnicodeToYandex(buf.Detach(), 11u, goodWord, 5u, written);
    UNIT_ASSERT_EQUAL(written, 5u);
    UNIT_ASSERT_EQUAL(buf[0], -15);
}

UNIT_TEST_SUITE_REGISTRATION(TFormToKeyConvertorTest);
