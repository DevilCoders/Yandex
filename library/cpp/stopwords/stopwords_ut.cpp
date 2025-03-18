#include <library/cpp/testing/unittest/registar.h>
#include <util/charset/wide.h>
#include <util/stream/str.h>
#include "stopwords.h"

class TTestStopWordsHash: public TTestBase {
    UNIT_TEST_SUITE(TTestStopWordsHash);
    UNIT_TEST(Test);
    UNIT_TEST_SUITE_END();

public:
    void Test() {
        TWordFilter wf;
        const TString stopwords = "Hi\nmE\nIT\nam";
        TStringInput input(stopwords);
        wf.InitStopWordsList(input);
        UNIT_ASSERT(wf.IsStopWord("hi"));
        UNIT_ASSERT(!wf.IsStopWord("Em"));
        UNIT_ASSERT(wf.IsStopWord("am"));
        UNIT_ASSERT(wf.IsStopWord("iT"));
        UNIT_ASSERT(wf.IsStopWord(u"hi"));
        UNIT_ASSERT(!wf.IsStopWord(u"Em"));
        UNIT_ASSERT(wf.IsStopWord(u"am"));
        UNIT_ASSERT(wf.IsStopWord(u"iT"));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TTestStopWordsHash);
