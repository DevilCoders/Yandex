#include <library/cpp/charset/recyr.hh>
#include <library/cpp/charset/wide.h>
#include <library/cpp/testing/unittest/registar.h>

#include <kernel/lemmer/dictlib/grammar_index.h>
#include <kernel/indexer/direct_text/dt.h>

#include "dtcreator.h"

using namespace NIndexerCore;

class TDirectTextCreatorTest: public TTestBase {
    UNIT_TEST_SUITE(TDirectTextCreatorTest);
        UNIT_TEST(TestSimple);
    UNIT_TEST_SUITE_END();
public:
    void TestSimple();
};

class TCallbackTest : public IDirectTextCallback2 {
public:
    void ProcessDirectText2(IDocumentDataInserter* /*inserter*/, const TDirectTextData2& directText, ui32) override {
        for (size_t i = 0; i < directText.EntryCount; ++i) {
            const TDirectTextEntry2& entry = directText.Entries[i];
            if (entry.Token) {
                const size_t toklen = entry.Token.size();
                TTempBuf buf(toklen + 1);
                WideToChar(entry.Token.data(), toklen + 1, buf.Data(), CODES_YANDEX);
                const char* token = buf.Data();
                Text += token;

                char Buffer[200];
                sprintf(Buffer, "[%d.%d] %s", TWordPosition::Break((i32)entry.Posting),
                                                  TWordPosition::Word((i32)entry.Posting), token);
                StemOutput += Buffer;

                for (ui32 j = 0; j < entry.LemmatizedTokenCount; ++j) {
                    const TLemmatizedToken& tf = entry.LemmatizedToken[j];
                    sprintf(Buffer, " [+%d] %s, %s, ", tf.FormOffset, tf.FormaText, tf.LemmaText);
                    StemOutput += Buffer;
                    StemOutput += sprint_grammar(tf.StemGram);
                    for (ui8 j = 0; j < tf.GramCount; j++) {
                        const char* flexGram = tf.FlexGrams[j];
                        StemOutput += ", ";
                        StemOutput += sprint_grammar(flexGram);
                    }
                }
                //if (strcmp(token, "рыбака") == 0) {
                //    inserter->StoreLemma("удильщик", "удильщика", 0, entry.Posting, LANG_UNK);
                //}
            }
            for (ui32 j = 0; j < entry.SpaceCount; j++) {
                TDirectTextSpace& space = entry.Spaces[j];
                TTempBuf buf(space.Length);
                WideToChar(space.Space, space.Length, buf.Data(), CODES_YANDEX);
                Text.append(buf.Data(), 0, space.Length);
            }
        }
        //inserter->StoreLiteralAttr("gooddoc", "1", 0);
    }
public:
    TString Text;
    TString StemOutput;
};

const char* const TestStemOutput = "\
[1.1] омонимия\
 [+0] омонимия, омонимия, S,f,inan, nom,sg\
[1.2] черно-белый\
 [+0] черно, черный, A, sg,brev,n\
 [+0] черно, черно, ADV, \
 [+0] черно-белый, черно-белый, A, acc,sg,plen,m,inan, nom,sg,plen,m\
 [+1] белый, белый, A, acc,sg,plen,m,inan, nom,sg,plen,m\
 [+1] белый, белый, S,famn,m,anim, nom,sg\
[1.4] день\
 [+0] день, день, S,m,inan, acc,sg, nom,sg\
[1.5] рыбака\
 [+0] рыбака, рыбак, S,m,anim, acc,sg, gen,sg\
";

void TDirectTextCreatorTest::TestSimple() {
    TUtf16String seq = u"омонимия: черно-белый день рыбака";
    const wchar16* const Text = seq.data();

    TCallbackTest callbackTest;

    TDTCreatorConfig cfg;
    TDirectTextCreator dtc(cfg, TLangMask(LANG_RUS, LANG_ENG), LANG_UNK);

    dtc.AddDoc(100, LANG_UNK);

    TWordPosition tokPos(0, 1, 1);
    TWideToken tok;
    tok.SubTokens.push_back(0, 0);
    TCharSpan& span = tok.SubTokens.back();
    span.Pos = 0;

    tok.Token = Text;
    tok.Leng = span.Len = 8;
    dtc.StoreForm(tok, 0, tokPos.DocLength());
    tokPos.Inc();

    dtc.StoreSpaces(Text+8, 1, 0);
    dtc.StoreSpaces(Text+9, 1, 0);

    TWideToken tok2;
    tok2.Token = Text + 10;
    tok2.Leng = 11;
    tok2.SubTokens.push_back(0, 5, TOKEN_WORD, TOKDELIM_MINUS);
    tok2.SubTokens.push_back(6, 5);
    dtc.StoreForm(tok2, 10, tokPos.DocLength());
    tokPos.Inc();
    tokPos.Inc();

    dtc.StoreSpaces(Text+21, 1, 0);

    tok.SubTokens.clear();
    tok.SubTokens.push_back(0, 0);
    tok.Token = Text + 22;
    tok.Leng = tok.SubTokens.back().Len = 4;
    dtc.StoreForm(tok, 22, tokPos.DocLength());
    tokPos.Inc();

    dtc.StoreSpaces(Text+26, 1, 0);

    tok.Token = Text + 27;
    tok.Leng = tok.SubTokens.back().Len = 6;
    dtc.StoreForm(tok, 27, tokPos.DocLength());
    tokPos.Inc();

    dtc.CommitDoc();
    dtc.ProcessDirectText(callbackTest, nullptr /*&inserter*/);
    dtc.ClearDirectText();

    UNIT_ASSERT_VALUES_EQUAL(RecodeFromYandex(CODES_UTF8, callbackTest.Text), WideToUTF8(seq));
    UNIT_ASSERT_VALUES_EQUAL(RecodeFromYandex(CODES_UTF8, callbackTest.StemOutput), TestStemOutput);
}

UNIT_TEST_SUITE_REGISTRATION(TDirectTextCreatorTest);
