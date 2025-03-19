#include "attryndex.h"

#include <library/cpp/testing/unittest/registar.h>
#include <kernel/keyinv/invkeypos/keychars.h>
#include <library/cpp/wordpos/wordpos.h>

class TAttrStackerTest : public TTestBase {
    UNIT_TEST_SUITE(TAttrStackerTest);
        UNIT_TEST(TestStoreUrl);
    UNIT_TEST_SUITE_END();
public:
    void TestStoreUrl();
};

UNIT_TEST_SUITE_REGISTRATION(TAttrStackerTest)

struct TTestInserter : public IDocumentDataInserter {
    size_t NumberCount;
    TTestInserter() : NumberCount(0) {}
    void StoreLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, TPosting pos, ELanguage lang) override {
//        Cout << GetWord(pos) << ": " << (int)lang << ", " << TUtf16String(lemma, lemmaLen) << ", "
//            << TUtf16String(form, formLen) << ", " << ((flags & FORM_TITLECASE) != 0) << ", " << ((flags & FORM_TRANSLIT) != 0) << Endl;
        UNIT_ASSERT(lemmaLen && formLen);
        UNIT_ASSERT(GetWord(pos) >= TWordPosition::FIRST_CHILD);
        UNIT_ASSERT(GetWord(pos) <= WORD_LEVEL_Max);
        if (IsDigit(*form)) {
            UNIT_ASSERT(!flags && lang == LANG_UNK);
            UNIT_ASSERT(TWtringBuf(lemma, lemmaLen) == TWtringBuf(form, formLen));
            ++NumberCount;
        } else
            UNIT_ASSERT(!(flags & ~(FORM_TITLECASE | FORM_TRANSLIT)));
    }
    void StoreLiteralAttr(const char*, const char*, TPosting) override { UNIT_FAIL("not implemented"); }
    void StoreLiteralAttr(const char*, const wchar16*, size_t, TPosting) override { UNIT_FAIL("not implemented"); }
    void StoreDateTimeAttr(const char*, time_t) override { UNIT_FAIL("not implemented"); }
    void StoreIntegerAttr(const char*, const char*, TPosting) override { UNIT_FAIL("not implemented"); }
    // сохранение при работе нумератора ключей, не подлежащих лемматизации
    void StoreKey(const char*, TPosting) override { UNIT_FAIL("not implemented"); }
    void StoreZone(const char*, TPosting, TPosting, bool) override { UNIT_FAIL("not implemented"); }
    void StoreArchiveZoneAttr(const char*, const wchar16*, size_t, TPosting) override { UNIT_FAIL("not implemented"); }
    void StoreTextArchiveDocAttr(const TString&, const TString&) override { UNIT_FAIL("not implemented"); }
    void StoreFullArchiveDocAttr(const TString&, const TString&) override { UNIT_FAIL("not implemented"); }
    void StoreErfDocAttr(const TString&, const TString&) override { UNIT_FAIL("not implemented"); }
    void StoreGrpDocAttr(const TString&, const TString&, bool) override { UNIT_FAIL("not implemented"); }
};

void TAttrStackerTest::TestStoreUrl() {
    TAttrStacker attrStacker;
    TTestInserter inserter;
    attrStacker.SetInserter(&inserter);
    attrStacker.StoreUrl("союз51.рф/vse-novosti#article-59", LANG_RUS);
    UNIT_ASSERT_VALUES_EQUAL(inserter.NumberCount, 2u);
}

