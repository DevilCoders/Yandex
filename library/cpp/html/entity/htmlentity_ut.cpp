#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/charset/wide.h>

class THtmlEntityTest: public TTestBase {
private:
    UNIT_TEST_SUITE(THtmlEntityTest);
    UNIT_TEST(Test);
    UNIT_TEST(TestMarkupSafety);
    UNIT_TEST(TestUnknownPlane);
    UNIT_TEST(TestHtLinkDecode);
    UNIT_TEST(TestDetectEntity);
    UNIT_TEST(TestDetectNumber);
    UNIT_TEST_SUITE_END();

public:
    void Test();
    void TestMarkupSafety();
    void TestUnknownPlane();
    void TestHtLinkDecode();
    void TestDetectEntity();
    void TestDetectNumber();
};

UNIT_TEST_SUITE_REGISTRATION(THtmlEntityTest);

TString HtEntDecodeToUtf(ECharset fromCharset, const TStringBuf& str) {
    TCharTemp wide(str.length() * 2); // if we have 1-byte charset that's all mapped to pairs
    size_t outLen = HtEntDecodeToChar(fromCharset, str.data(), str.length(), wide.Data());
    return WideToUTF8(wide.Data(), outLen);
}

TString TestHtEntDecodeToUtf8(ECharset fromCharset, const TStringBuf& str) {
    TTempBuf tmp(str.length() * 4);
    size_t outLen = HtEntDecodeToUtf8(fromCharset, str.data(), str.length(), tmp.Data(), tmp.Left());
    return TString(tmp.Data(), outLen);
}

void TestHtEntDecodeToUtf8TooSmallOutput(ECharset fromCharset, const TStringBuf& str) {
    const size_t smallBufSize = 1;
    TTempBuf tmp(smallBufSize);
    size_t outLen = HtEntDecodeToUtf8(fromCharset, str.data(), str.length(), tmp.Data(), smallBufSize);
    UNIT_ASSERT_C(outLen == 0 || outLen == smallBufSize, "outLen = " << outLen);
}

TString TestHtEntDecodeAsciiCompatToUtf8(ECharset fromCharset, const TStringBuf& str) {
    TTempBuf tmp(str.length() * 4);
    return TString(HtTryEntDecodeAsciiCompat(str, tmp.Data(), tmp.Left(), fromCharset, CODES_UTF8));
}

void THtmlEntityTest::Test() {
    const TStringBuf BROKEN_RUNE_UTF = "\xef\xbf\xbd";
    struct THtmlEntityTestStruct {
        const TStringBuf Html;
        const TStringBuf Str;
    } tests[] = {
        // positive
        {"&lt;&apos;&amp;&quot;&gt;", "<'&\">"},
        {"&lt;hello&gt;", "<hello>"},
        {"&#65;", "A"},
        {"&#65&#66", "AB"},
        {"&#x41;", "A"},
        {"&#x41&#x42", "AB"},
        {"&#X41;", "A"},
        {"&#4f;", "\x04"
                  "f;"},
        // negative
        {"&#xZ;", "&#xZ;"},
        {"&#Z;", "&#Z;"},
        {"&fuflo;", "&fuflo;"},
        {"&verylongentity", "&verylongentity"},
        {"&123;", "&123;"},
        // number termination
        {"&#65", "A"},
        {"&#65a", "Aa"},
        {"&#x65a", "\xd9\x9a"},
        {"&-1;", "&-1;"},
        // long numbers
        {"&#xfffffffffffffffff;", BROKEN_RUNE_UTF},
        {"&#xffffffff;", BROKEN_RUNE_UTF},
        {"&#000000000000000000;", BROKEN_RUNE_UTF},
        {"&#000065;", "A"},
        {"&#000000000000000065;", "A"},
        {"&#000000000000000000000000000000000000000000000000000000000000000000000000000065q", "Aq"},
        // hex
        {"&#x;", "&#x;"},
        {"&#x9;", "\x9"},
        {"&#x99", "\xe2\x84\xa2"},
        {"&#x999", "\xe0\xa6\x99"},
        {"&#x9999", "\xe9\xa6\x99"},
        {"&#x99999", "\xf2\x99\xa6\x99"},
        {"&#x999999", BROKEN_RUNE_UTF},
        {"&#x9999999", BROKEN_RUNE_UTF},
        {"&#x99999999", BROKEN_RUNE_UTF},
        // dec
        {"&#;", "&#;"},
        {"&#9", "\x9"},
        {"&#99", "c"},
        {"&#999", "\xcf\xa7"},
        {"&#9999", "\xe2\x9c\x8f"},
        {"&#99999", "\xf0\x98\x9a\x9f"},
        {"&#999999", "\xf3\xb4\x88\xbf"},
        {"&#9999999", BROKEN_RUNE_UTF},
        {"&#99999999", BROKEN_RUNE_UTF},
        {"&#999999999", BROKEN_RUNE_UTF},
    };

    TStringStream diff;
    for (int i = 0; i < (int)Y_ARRAY_SIZE(tests); ++i) {
        const TStringBuf& html = tests[i].Html;
        const TStringBuf& ok = tests[i].Str;

        TString result = HtEntDecodeToUtf(CODES_UTF8, html);
        if (result != ok)
            diff << "line=" << __LINE__ << " iter=" << i << ": " << html << " -> '" << result << "' (expected: '" << ok << "')\n";

        TString result2 = TestHtEntDecodeToUtf8(CODES_UTF8, html);
        if (result2 != ok) {
            diff << "line=" << __LINE__ << " iter=" << i << ": " << html << " -> '" << result2 << "' (expected: '" << ok << "')\n";
        }

        TString result3 = TestHtEntDecodeAsciiCompatToUtf8(CODES_UTF8, html);
        if (result3 != ok)
            diff << "line=" << __LINE__ << " iter=" << i << ": " << html << " -> '" << result3 << "' (expected: '" << ok << "')\n";

        TestHtEntDecodeToUtf8TooSmallOutput(CODES_UTF8, html);
    }
    if (!diff.empty())
        UNIT_FAIL("\n" + diff.Str());

    const TStringBuf src1 = "\xad\xb5\xa1\xa5\xcb\xde\xce\xe3";
    TString r2 = HtEntDecodeToUtf(CODES_EUC_JP, src1);
    UNIT_ASSERT_VALUES_EQUAL(r2, TestHtEntDecodeToUtf8(CODES_EUC_JP, src1));
    UNIT_ASSERT(r2.size() <= 8 * 6);

    TestHtEntDecodeToUtf8TooSmallOutput(CODES_EUC_JP, src1);

    {
        const TString test("text&vsupne;text");
        TString res = HtEntDecodeToUtf(CODES_UTF8, test);
        UNIT_ASSERT_VALUES_EQUAL(res.size(), 14);
    }
    {
        const TString test("&lt;text");
        const unsigned char* s = (const unsigned char*)test.data();
        const unsigned char* r = (const unsigned char*)test.data();
        wchar32 c = HtEntOldDecodeStep(CODES_UTF8, s, test.size(), nullptr);
        UNIT_ASSERT_VALUES_EQUAL(c, 60);
        UNIT_ASSERT(r + 4 == s);
        c = HtEntOldDecodeStep(CODES_UTF8, s, test.size(), nullptr);
        UNIT_ASSERT_VALUES_EQUAL(c, 't');
    }
    {
        const TString test("&vsupne;text");
        const unsigned char* s = (const unsigned char*)test.data();
        wchar32 c = HtEntOldDecodeStep(CODES_UTF8, s, test.size(), nullptr);
        UNIT_ASSERT_VALUES_EQUAL(c, '&');
    }
}

void THtmlEntityTest::TestMarkupSafety() {
    // To safely parse html we need to be sure what this symbols always remain at their positions in all encodings
    TString symbols = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789<>\"' \t\n\r -=+_?&;!";
    TUtf16String wideSymbols = UTF8ToWide(symbols);

    for (int enc = 0; enc < CODES_MAX; ++enc) {
        ECharset code = static_cast<ECharset>(enc);
        if (code == CODES_UNKNOWNPLANE || code == CODES_TDS565 || code == CODES_UTF_16BE || code == CODES_UTF_16LE)
            // UNKNOWNPLANE is special single byte encoding what maps all symbols to unknown plane
            // TDS565 is some stupid turkmenian encoding %/
            continue;

        UNIT_ASSERT_VALUES_EQUAL(CharToWide(symbols, code), wideSymbols);
    }
}

void THtmlEntityTest::TestUnknownPlane() {
    // CODES_UNKNOWNPLANE must map all symbols except zero to unknown plane
    const CodePage& page = *CodePageByCharset(CODES_UNKNOWNPLANE);

    UNIT_ASSERT_VALUES_EQUAL(page.unicode[0], 0u);

    for (ui64 i = 1; i < 256; ++i) {
        UNIT_ASSERT_VALUES_EQUAL(0xf000 + i, page.unicode[i]);
    }
}

void THtmlEntityTest::TestHtLinkDecode() {
    {
        TString url = "/news/tag/プログラミング";
        ECharset encoding = CharsetByName("EUC-JP");
        TString buf = TString::Uninitialized(56);
        bool decoded = HtLinkDecode(url.data(), buf.begin(), buf.size(), encoding);

        UNIT_ASSERT_VALUES_EQUAL(decoded, false);
    }
    {
        TString url = "index/?a=ddd&b=&amp;data";
        ECharset encoding = CharsetByName("CODES_UTF8");
        TString buf = TString::Uninitialized(56);
        size_t written = 0;
        bool decoded = HtLinkDecode(url.data(), buf.begin(), buf.size(), written, encoding);

        UNIT_ASSERT_VALUES_EQUAL(decoded, true);
        UNIT_ASSERT_VALUES_EQUAL(written, 20);
    }
    {
        TString url = "index/?a=ddd&b=&ampdata";
        ECharset encoding = CharsetByName("CODES_UTF8");
        TString buf = TString::Uninitialized(56);
        size_t written = 0;
        bool decoded = HtLinkDecode(url.data(), buf.begin(), buf.size(), written, encoding);

        UNIT_ASSERT_VALUES_EQUAL(decoded, true);
        UNIT_ASSERT_VALUES_EQUAL(written, 23);
    }
    {
        TString url = "index/?a=ddd&b=&#38;data";
        ECharset encoding = CharsetByName("CODES_UTF8");
        TString buf = TString::Uninitialized(56);
        size_t written = 0;
        bool decoded = HtLinkDecode(url.data(), buf.begin(), buf.size(), written, encoding);

        UNIT_ASSERT_VALUES_EQUAL(decoded, true);
        UNIT_ASSERT_VALUES_EQUAL(written, 20);
    }
    {
        TString url = "index/?a=ddd&b=&#x26;data";
        ECharset encoding = CharsetByName("CODES_UTF8");
        TString buf = TString::Uninitialized(56);
        size_t written = 0;
        bool decoded = HtLinkDecode(url.data(), buf.begin(), buf.size(), written, encoding);

        UNIT_ASSERT_VALUES_EQUAL(decoded, true);
        UNIT_ASSERT_VALUES_EQUAL(written, 20);
    }
    {
        TString url = "index/?a=a%b";
        ECharset encoding = CharsetByName("CODES_UTF8");
        TString buf = TString::Uninitialized(56);
        size_t written = 0;
        bool decoded = HtLinkDecode(url.data(), buf.begin(), buf.size(), written, encoding);

        UNIT_ASSERT_VALUES_EQUAL(decoded, true);
        UNIT_ASSERT_VALUES_EQUAL(written, 12);
    }
}

void THtmlEntityTest::TestDetectEntity() {
    {
        TStringBuf str = "&lt;";

        TEntity entity;
        UNIT_ASSERT(HtTryDecodeEntity(str.data(), str.size() - 1, &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, str.size() - 1);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 60);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint2, 0);

        char* str2 = new char[str.size() - 1];
        memcpy(str2, str.data(), str.size() - 1);
        UNIT_ASSERT(HtTryDecodeEntity(str2, str.size() - 1, &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, str.size() - 1);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 60);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint2, 0);
        delete[] str2;
    }
    {
        const TString inpt("&vsupne;");
        TEntity entity;
        UNIT_ASSERT(HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, 8);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 8843);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint2, 65024);
    }
    {
        const TString inpt("&vsupne");
        TEntity entity;
        UNIT_ASSERT(!HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
    }
}

void THtmlEntityTest::TestDetectNumber() {
    {
        const TString inpt("&#124");
        TEntity entity;
        UNIT_ASSERT(HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, 5);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 124);
    }
    {
        const TString inpt("&#abc124");
        TEntity entity;
        UNIT_ASSERT(!HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
    }
    {
        const TString inpt("&#124;");
        TEntity entity;
        UNIT_ASSERT(HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, 6);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 124);
    }
    {
        const TString inpt("&#124 2a");
        TEntity entity;
        UNIT_ASSERT(HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, 5);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 124);
    }
    {
        const TString inpt("&#7x");
        TEntity entity;
        UNIT_ASSERT(HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, 3);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 7);
    }
    {
        const TString inpt("&#qqq7x");
        TEntity entity;
        UNIT_ASSERT(!HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
    }
    {
        const TString inpt("&#124334a");
        TEntity entity;
        UNIT_ASSERT(HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, 8);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 124334);
    }
    {
        const TString inpt("&#x7crr");
        TEntity entity;
        UNIT_ASSERT(HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, 5);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 124);
    }
    {
        const TString inpt("&#x7");
        TEntity entity;
        UNIT_ASSERT(HtTryDecodeEntity(inpt.data(), inpt.size(), &entity));
        UNIT_ASSERT_VALUES_EQUAL(entity.Len, 4);
        UNIT_ASSERT_VALUES_EQUAL(entity.Codepoint1, 7);
    }
}
