#include "relalternate.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/split.h>
#include <util/string/vector.h>

class TRelAlternateTest: public TTestBase {
    UNIT_TEST_SUITE(TRelAlternateTest);
    UNIT_TEST(TestConcatenateLangHref);
    UNIT_TEST(TestSplitLangHref);
    UNIT_TEST(TestAppendRelAlternateBuffer);
    UNIT_TEST(TestAppendRelAlternateStroka);
    UNIT_TEST(TestSplitRelAlternate);
    UNIT_TEST(TestSplitAlternateHreflang);
    UNIT_TEST_SUITE_END();

public:
    void TestConcatenateLangHref();
    void TestSplitLangHref();
    void TestAppendRelAlternateBuffer();
    void TestAppendRelAlternateStroka();
    void TestSplitRelAlternate();
    void TestSplitAlternateHreflang();
};

void TRelAlternateTest::TestConcatenateLangHref() {
    TString valueBuf("http://example.es");
    char zone[] = "es_br";
    valueBuf = NRelAlternate::ConcatenateLangHref(
        TStringBuf(zone, 5),
        valueBuf);
    UNIT_ASSERT_VALUES_EQUAL(valueBuf, "es_br http://example.es");
}

void TRelAlternateTest::TestSplitLangHref() {
    {
        TString relAlternate("");
        TStringBuf langHref(relAlternate.data(), relAlternate.size());
        TStringBuf lang;
        TStringBuf href;
        NRelAlternate::SplitLangHref(langHref, lang, href);
        UNIT_ASSERT(lang.empty());
        UNIT_ASSERT(href.empty());
    }
    {
        TString relAlternate(" http://example.es");
        TStringBuf langHref(relAlternate.data(), relAlternate.size());
        TStringBuf lang;
        TStringBuf href;
        NRelAlternate::SplitLangHref(langHref, lang, href);
        UNIT_ASSERT(lang.empty());
        UNIT_ASSERT_VALUES_EQUAL(href, "http://example.es");
    }
    {
        TString relAlternate("http://example.es");
        TStringBuf langHref(relAlternate.data(), relAlternate.size());
        TStringBuf lang;
        TStringBuf href;
        NRelAlternate::SplitLangHref(langHref, lang, href);
        UNIT_ASSERT(lang.empty());
        UNIT_ASSERT_VALUES_EQUAL(href, "http://example.es");
    }
    {
        TString relAlternate("es_br http://example.es");
        TStringBuf langHref(relAlternate.data(), relAlternate.size());
        TStringBuf lang;
        TStringBuf href;
        NRelAlternate::SplitLangHref(langHref, lang, href);
        UNIT_ASSERT_VALUES_EQUAL(lang, "es_br");
        UNIT_ASSERT_VALUES_EQUAL(href, "http://example.es");
    }
}

void TRelAlternateTest::TestAppendRelAlternateBuffer() {
    {
        TString propValues("");
        TString relAlternate("");
        TBuffer relAlternateBuf;
        relAlternateBuf.Append(relAlternate.data(), relAlternate.size());
        NRelAlternate::AppendRelAlternate(relAlternateBuf, propValues);
        UNIT_ASSERT_VALUES_EQUAL(TString(relAlternateBuf.Data(), relAlternateBuf.Size()), TString(""));
    }
    {
        TString propValues("es_br http://example.es\ten_us http://example.org");
        TString relAlternate("");
        TBuffer relAlternateBuf;
        relAlternateBuf.Append(relAlternate.data(), relAlternate.size());
        NRelAlternate::AppendRelAlternate(relAlternateBuf, propValues);
        UNIT_ASSERT_VALUES_EQUAL(TString(relAlternateBuf.Data(), relAlternateBuf.Size()), TString("es_br http://example.es\ten_us http://example.org").append('\0'));
    }
    {
        TString propValues("");
        TString relAlternate("ru_ru http://example.ru");
        TBuffer relAlternateBuf;
        relAlternateBuf.Append(relAlternate.data(), relAlternate.size());
        NRelAlternate::AppendRelAlternate(relAlternateBuf, propValues);
        UNIT_ASSERT_VALUES_EQUAL(TString(relAlternateBuf.Data(), relAlternateBuf.Size()), TString("ru_ru http://example.ru").append('\0'));
    }
    {
        TString propValues("es_br http://example.es\ten_us http://example.org");
        TString relAlternate("ru_ru http://example.ru");
        TBuffer relAlternateBuf;
        relAlternateBuf.Append(relAlternate.data(), relAlternate.size());
        NRelAlternate::AppendRelAlternate(relAlternateBuf, propValues);
        UNIT_ASSERT_VALUES_EQUAL(TString(relAlternateBuf.Data(), relAlternateBuf.Size()), TString("ru_ru http://example.ru\tes_br http://example.es\ten_us http://example.org").append('\0'));
    }
}

void TRelAlternateTest::TestAppendRelAlternateStroka() {
    {
        TString propValues("");
        TString relAlternate("");
        NRelAlternate::AppendRelAlternate(relAlternate, propValues);
        UNIT_ASSERT_VALUES_EQUAL(relAlternate, "");
    }
    {
        TString propValues("es_br http://example.es\ten_us http://example.org");
        TString relAlternate("");
        NRelAlternate::AppendRelAlternate(relAlternate, propValues);
        UNIT_ASSERT_VALUES_EQUAL(relAlternate, "es_br http://example.es\ten_us http://example.org");
    }
    {
        TString propValues("");
        TString relAlternate("ru_ru http://example.ru");
        NRelAlternate::AppendRelAlternate(relAlternate, propValues);
        UNIT_ASSERT_VALUES_EQUAL(relAlternate, "ru_ru http://example.ru");
    }
    {
        TString propValues("es_br http://example.es\ten_us http://example.org");
        TString relAlternate("ru_ru http://example.ru");
        NRelAlternate::AppendRelAlternate(relAlternate, propValues);
        UNIT_ASSERT_VALUES_EQUAL(relAlternate, "ru_ru http://example.ru\tes_br http://example.es\ten_us http://example.org");
    }
}

void TRelAlternateTest::TestSplitRelAlternate() {
    {
        TString relAlternate("ru_ru http://example.ru\tes_br http://example.es\ten_us http://example.org");
        TVector<TStringBuf> result = NRelAlternate::SplitRelAlternate(TStringBuf(relAlternate.data(), relAlternate.size()));
        TVector<TStringBuf> reference = {"ru_ru http://example.ru", "es_br http://example.es", "en_us http://example.org"};
        UNIT_ASSERT_VALUES_EQUAL(result, reference);
    }
    {
        TString relAlternate("ru_ru http://example.ru\t");
        TVector<TStringBuf> result = NRelAlternate::SplitRelAlternate(TStringBuf(relAlternate.data(), relAlternate.size()));
        TVector<TStringBuf> reference = {"ru_ru http://example.ru"};
        UNIT_ASSERT_VALUES_EQUAL(result, reference);
    }
}

void TRelAlternateTest::TestSplitAlternateHreflang() {
    {
        TString relAlternate("ru_ru http://example.ru\tes_br http://example.es\ten_us http://example.org");
        NRelAlternate::THrefLangVector result = NRelAlternate::SplitAlternateHreflang(TStringBuf(relAlternate.data(), relAlternate.size()));
        NRelAlternate::THrefLangVector reference = {
            {"http://example.ru", "ru_ru"},
            {"http://example.es", "es_br"},
            {"http://example.org", "en_us"}};
        Cerr << "compare!!" << Endl;
        UNIT_ASSERT_VALUES_EQUAL(result.size(), reference.size());
        auto resIt = result.begin();
        auto refIt = reference.begin();
        for (; (resIt != result.end()) && (refIt != reference.end()); ++resIt, ++refIt) {
            UNIT_ASSERT_VALUES_EQUAL(resIt->Href, refIt->Href);
            UNIT_ASSERT_VALUES_EQUAL(resIt->Lang, refIt->Lang);
        }
    }
}

UNIT_TEST_SUITE_REGISTRATION(TRelAlternateTest);
