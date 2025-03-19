#include <kernel/search_types/search_types.h>
#include <library/cpp/testing/unittest/registar.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <util/generic/algorithm.h>
#include <util/random/shuffle.h>
#include <util/charset/wide.h>
#include "invcreator.h"

using namespace NIndexerCore;

class TTestKey {
public:
    char Text[MAXKEY_BUF];
    TVector<SUPERLONG> Hits;
    explicit TTestKey(const char* text) {
        UNIT_ASSERT(strlen(text) < MAXKEY_BUF);
        strcpy(Text, text);
    }
    void AddHits(const SUPERLONG* p, size_t n) {
        Hits.insert(Hits.end(), p, p + n);
    }
};

class TTestPosting {
    TPosting Val;
public:
    TTestPosting(int b, int w) {
        SetPosting(Val, b, w);
    }
    TPosting operator*() const {
        return Val;
    }
    TPosting operator++() {
        Val = PostingInc(Val);
        return Val;
    }
    TPosting operator++(int) {
        const TPosting ret = Val;
        Val = PostingInc(Val);
        return ret;
    }
};

class TTestStorage : public IYndexStorageFactory, private IYndexStorage {
    TVector<TTestKey> Keys;
    SUPERLONG PrevPos; // for checking positions

    void GetStorage(IYndexStorage** storage, IYndexStorage::FORMAT) override {
        *storage = this;
    }
    void ReleaseStorage(IYndexStorage*) override {
        // storage collects all keys
    }
    void StorePositions(const char* textPointer, SUPERLONG* filePositions, size_t filePosCount) override {
        if (Keys.empty() || strcmp(Keys.back().Text, textPointer) != 0) {
            Keys.push_back(TTestKey(textPointer));
            PrevPos = 0;
        } else
            UNIT_ASSERT(strcmp(Keys.back().Text, textPointer) < 0); // check key sorting
        UNIT_ASSERT(filePosCount);
        for (size_t i = 0; i < filePosCount; ++i) {
            UNIT_ASSERT(PrevPos < filePositions[i]); // check position sorting
            PrevPos = filePositions[i];
        }
        Keys.back().AddHits(filePositions, filePosCount);
    }
public:
    TTestStorage()
        : PrevPos(0)
    {
    }
    void PrintIndex() const {
        for (size_t i = 0; i < Keys.size(); ++i) {
            const TTestKey& key = Keys[i];
            TKeyReader reader(key.Text);
            Cout << reader.GetPrefix() << reader.GetLemma() << " " << (int)reader.GetLang() << " ";
            if (reader.GetFormCount()) {
                Cout << "( ";
                for (size_t j = 0; j < reader.GetFormCount(); ++j) {
                    const TKeyReader::TForm& f = reader.GetForm(j);
                    Cout << f.Text;
                    if (f.Flags || f.Joins)
                        Cout << ":" << (int)f.Flags << ":" << (int)f.Joins;
                    Cout << " ";
                }
                Cout << ") ";
            }
            for (size_t j = 0; j < key.Hits.size(); ++j) {
                const SUPERLONG h = key.Hits[j];
                Cout << TWordPosition::Doc(h) << "." << TWordPosition::Break(h) << "."
                    << TWordPosition::Word(h) << "." << TWordPosition::Form(h) << " ";
            }
            Cout << Endl;
        }
    }
    void CheckForm(const char* lemma, const char* form, ui8 lang, size_t hitCount) {
        for (size_t i = 0; i < Keys.size(); ++i) {
            const TTestKey& k = Keys[i];
            TKeyReader reader(k.Text);
            if (strcmp(reader.GetLemma(), lemma) == 0 && reader.GetLang() == lang) {
                for (size_t j = 0; j < reader.GetFormCount(); ++j) {
                    const TKeyReader::TForm& f = reader.GetForm(j);
                    if (strcmp(f.Text, form) == 0) {
                        size_t n = 0;
                        for (TVector<SUPERLONG>::const_iterator it = k.Hits.begin(); it != k.Hits.end(); ++it) {
                            if (TWordPosition::Form(*it) == j)
                                ++n;
                        }
//                        Cout << lemma << " (" << (int)lang << ") " << form << ":" << (int)flags << ":" << (int)joins << " ";
//                        Cout.Flush();
                        UNIT_ASSERT_VALUES_EQUAL(n, hitCount);
                        return;
                    }
                }
            }
        }
        UNIT_FAIL(Sprintf("NOT FOUND: %s (%d) %s %" PRIu64, lemma, (int)lang, form, hitCount));
    }
    void CheckKey(const char* keyText, const TVector<TPosting>& hits) {
        for (size_t i = 0; i < Keys.size(); ++i) {
            if (strcmp(Keys[i].Text, keyText) == 0) {
                for (size_t j = 0; j < hits.size(); ++j) {
//                    Cout << Keys[i].Text << ": " << TWordPosition::Break(Keys[i].Hits[j])
//                        << "." << TWordPosition::Word(Keys[i].Hits[j]) << "." << TWordPosition::Form(Keys[i].Hits[j]) << " ?= "
//                        << TWordPosition::Break(hits[j]) << "." << TWordPosition::Word(hits[j]) << "." << TWordPosition::Form(hits[j]) << " ";
//                    Cout.Flush();
                    UNIT_ASSERT_VALUES_EQUAL(Keys[i].Hits[j] & BREAKWORDRELEV_LEVEL_Mask, hits[j]);
                }
            }
        }
    }
};

class TInvCreatorTest : public TTestBase {
    UNIT_TEST_SUITE(TInvCreatorTest);
        UNIT_TEST(Test);
        UNIT_TEST(Test2);
    UNIT_TEST_SUITE_END();
public:
    void Test();
    void Test2();
private:
    void InitToken(TLemmatizedToken& token, const char* lemmaText, const char* formaText, ui8 lang) {
        token.LemmaText = lemmaText;
        token.FormaText = formaText;
        token.Lang = lang;
    }
    TPosting GetPosting(int b, int w) {
        TPosting p;
        SetPosting(p, b, w);
        return p;
    }
};

UNIT_TEST_SUITE_REGISTRATION(TInvCreatorTest)

void TInvCreatorTest::Test() {
    TInvCreatorConfig config(1);
    TInvCreator creator(config);
    TTestPosting pos(1, 1);
    const char words[] = "a\0b\0c\0d\0e\0f\0g\0h\0i\0j\0k\0l\0m\0n\0o\0p\0q\0r\0s\0t\0u\0v\0w\0x\0y\0z";

    creator.AddDoc();

    TLemmatizedToken token;
    typedef TVector<const char*> TForms; // a text where all forms has the same lemma
    TForms forms;
    for (size_t i = 0; i < sizeof(words); i += 2) {
        const char* const p = words + i;
        forms.insert(forms.end(), ('z' + 1 - *p), p); // 'a' - 26 positions, 'z' - 1 position
    }
    Shuffle(forms.begin(), forms.end());
    TVector<TPosting> abcd_postings, o_postings, p_postings, q_postings;
    for (TForms::const_iterator it = forms.begin(); it != forms.end(); ++it) {
        InitToken(token, "a", *it, LANG_ENG);
        if (**it >= 'a' && **it <= 'd')
            abcd_postings.push_back(*pos);
        else if (**it == 'o')
            o_postings.push_back(*pos);
        else if (**it == 'p')
            p_postings.push_back(*pos);
        else if (**it == 'q')
            q_postings.push_back(*pos);
        creator.StoreCachedLemma(token, pos++);
    }
    // add external lemmas for "oo", "pp" and "qq"
    const wchar16 a_lemma[] = { 'a', 0 };
    const wchar16 oo_form[] = { 'o', 'o', 0 };
    const wchar16 pp_form[] = { 'p', 'p', 0 };
    const wchar16 q_lemma[] = { 'q', 0 };
    const wchar16 qq_form[] = { 'q', 'q', 0 };
    for (TVector<TPosting>::const_iterator it = o_postings.begin(); it != o_postings.end(); ++it)
        creator.StoreExternalLemma(a_lemma, 1, oo_form, 2, 0, LANG_ENG, *it);
    size_t n_pp = 0;
    for (TVector<TPosting>::const_iterator it = p_postings.begin(); it != p_postings.end(); ++it) {
            creator.StoreExternalLemma(a_lemma, 1, pp_form, 2, 0, LANG_ENG, *it);
            ++n_pp;
    }
    for (TVector<TPosting>::const_iterator it = q_postings.begin(); it != q_postings.end(); ++it)
        creator.StoreExternalLemma(q_lemma, 1, qq_form, 2, 0, LANG_UNK, *it);

    for (TVector<TPosting>::const_iterator it = abcd_postings.begin(); it != abcd_postings.end(); ++it)
        creator.StoreZone("abcd", *it, PostingInc(*it));

    Shuffle(o_postings.begin(), o_postings.end());
    const wchar16 a_val[] = { 'x', 'y', 'z', 0 };
    for (TVector<TPosting>::const_iterator it = o_postings.begin(); it != o_postings.end(); ++it)
        creator.StoreAttr(DTAttrSearchLiteral, "a", a_val, *it);

    Shuffle(p_postings.begin(), p_postings.end());
    const wchar16 b_val[] = { '1', '2', '3', 0 };
    for (TVector<TPosting>::const_iterator it = p_postings.begin(); it != p_postings.end(); ++it)
        creator.StoreAttr(DTAttrSearchInteger, "b", b_val, *it);

    Shuffle(q_postings.begin(), q_postings.end());
    for (TVector<TPosting>::const_iterator it = q_postings.begin(); it != q_postings.end(); ++it)
        creator.StoreKey("##TEST_KEY", *it);

    creator.CommitDoc(0, 1);
    UNIT_ASSERT(creator.IsFull());
    TTestStorage storage;
    creator.MakePortion(&storage, true);

//    storage.PrintIndex();

    for (TForms::const_iterator it = forms.begin(); it != forms.end(); ++it)
        storage.CheckForm("a", *it, LANG_ENG, 'z' + 1 - **it);
    storage.CheckForm("a", "oo", LANG_ENG, o_postings.size());
    storage.CheckForm("a", "pp", LANG_ENG, n_pp);
    storage.CheckForm("q", "qq", LANG_UNK, q_postings.size());

    Sort(abcd_postings.begin(), abcd_postings.end());
    storage.CheckKey("(abcd", abcd_postings);
    TVector<TPosting> abcd_end_postings;
    for (TVector<TPosting>::const_iterator it = abcd_postings.begin(); it != abcd_postings.end(); ++it) {
        TPosting pos(PostingInc(*it));
        SetPostingRelevLevel(pos, BEST_RELEV);
        abcd_end_postings.push_back(pos);
    }
    storage.CheckKey(")abcd", abcd_end_postings);

    Sort(o_postings.begin(), o_postings.end());
    storage.CheckKey("#a=\"xyz", o_postings);

    Sort(p_postings.begin(), p_postings.end());
    storage.CheckKey("#b=0000000123", p_postings);

    Sort(q_postings.begin(), q_postings.end());
    storage.CheckKey("##TEST_KEY", q_postings);
}

const TUtf16String first(u"first");
const TUtf16String second(u"second");

inline void StoreLine(TInvCreator& creator, int doc, int br) {
    creator.AddDoc();
    TTestPosting pos(br, 1);
    creator.StoreExternalLemma(first.data(), first.size(), first.data(), first.size(), 0, 2, pos++);
    creator.StoreExternalLemma(second.data(), second.size(), second.data(), second.size(), 0, 2, pos++);
    creator.CommitDoc(0, doc);
}

void TInvCreatorTest::Test2() {
    TInvCreatorConfig config(5);
    TInvCreator creator(config);
    StoreLine(creator, 42, 5);
    StoreLine(creator, 3, 2);
    StoreLine(creator, 42, 7);
    TTestStorage storage;
    creator.MakePortion(&storage, true);
    storage.CheckForm("first", "first", 2, 3);
    storage.CheckForm("second", "second", 2, 3);
}
