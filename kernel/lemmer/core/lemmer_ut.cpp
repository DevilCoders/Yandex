#include "lemmer.h"
#include "lemmeraux.h"

#include <library/cpp/testing/unittest/registar.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>
#include <kernel/lemmer/dictlib/grammar_enum.h>

#include <util/charset/wide.h>

class TTestYLemma: public TTestBase {
    UNIT_TEST_SUITE(TTestYLemma);
    UNIT_TEST(TestHasGram);
    UNIT_TEST_SUITE_END();

public:
    void TestHasGram() {
        auto lemma = TYandexLemma();
        auto setter = NLemmerAux::TYandexLemmaSetter(lemma);

        using NTGrammarProcessing::tg2ch;

        TString flexGr1;
        flexGr1 += tg2ch(gNominative);
        flexGr1 += tg2ch(gSingular);

        TString flexGr2;
        flexGr2 += tg2ch(gNominative);
        flexGr2 += tg2ch(gPlural);

        setter.AddFlexGr(flexGr1.c_str());
        setter.AddFlexGr(flexGr2.c_str());

        UNIT_ASSERT(lemma.HasFlexGram(gNominative));
        UNIT_ASSERT(lemma.HasFlexGram(gPlural));
        UNIT_ASSERT(!lemma.HasFlexGram(gAdjective));

        UNIT_ASSERT(lemma.HasGram(gSingular));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TTestYLemma);
