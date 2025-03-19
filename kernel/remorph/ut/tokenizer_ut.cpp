#include <kernel/remorph/tokenizer/tokenizer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>
#include <util/charset/wide.h>
#include <util/string/split.h>
#include <util/string/vector.h>

using namespace NToken;

typedef void (*PFN_VALIDATOR)(const TVector<TSentenceInfo>& ss);

struct TValidationCallback : public ITokenizerCallback {
    PFN_VALIDATOR Validator;
    TVector<TSentenceInfo> Sentences;
    TValidationCallback(PFN_VALIDATOR pfv)
        : Validator(pfv)
    {
    }

    ~TValidationCallback() override {
        Validator(Sentences);
    }

    void OnSentence(const TSentenceInfo& sentInfo) override {
        Sentences.push_back(sentInfo);
    }
};

void TestTextTokenization(const TStringBuf& text, PFN_VALIDATOR pfv, const TTokenizeOptions& opts) {
    TValidationCallback cb(pfv);
    TokenizeText(cb, TUtf16String::FromUtf8(text), opts);
}

void TestStreamTokenization(const TString& text, PFN_VALIDATOR pfv, const TTokenizeOptions& opts) {
    TValidationCallback cb(pfv);
    TStringInput in(text);
    TokenizeStream(cb, in, CODES_UTF8, opts);
}

#define TOKENIZER_TEST(name, text, opts) \
    void Check ## name(const TVector<TSentenceInfo>& ss); \
    Y_UNIT_TEST(Test ## name) { \
        TestTextTokenization(text, &Check ## name, opts); \
    } \
    void Check ## name(const TVector<TSentenceInfo>& ss)

#define TOKENIZER_TEST_STREAM(name, text, opts) \
    void Check ## name(const TVector<TSentenceInfo>& ss); \
    Y_UNIT_TEST(Test ## name) { \
        TestStreamTokenization(text, &Check ## name, opts); \
    } \
    void Check ## name(const TVector<TSentenceInfo>& ss)

#define ASSERT_SENT(sentInfo, start, end, text, countOfTokens) \
    UNIT_ASSERT_EQUAL(sentInfo.Pos, decltype(sentInfo.Pos)(start, end));   \
    UNIT_ASSERT_EQUAL(sentInfo.Text, TUtf16String::FromUtf8(text)); \
    UNIT_ASSERT_EQUAL(sentInfo.Tokens.size(), countOfTokens);

#define ASSERT_WORD(sentInfo, tokenNum, origOffset, text, spaceBefore) \
    UNIT_ASSERT(sentInfo.Tokens[tokenNum].IsNormalToken()); \
    UNIT_ASSERT_EQUAL(TUtf16String(sentInfo.Text.data() + sentInfo.Tokens[tokenNum].TokenOffset, sentInfo.Tokens[tokenNum].Length), TUtf16String::FromUtf8(text)); \
    UNIT_ASSERT_EQUAL(sentInfo.Tokens[tokenNum].OriginalOffset, origOffset); \
    UNIT_ASSERT_EQUAL(sentInfo.Tokens[tokenNum].Punctuation.size(), 0); \
    UNIT_ASSERT_EQUAL(sentInfo.Tokens[tokenNum].SpaceBefore, spaceBefore);

#define ASSERT_PUNCT(sentInfo, tokenNum, origOffset, text, spaceBefore) \
    UNIT_ASSERT(!sentInfo.Tokens[tokenNum].IsNormalToken()); \
    UNIT_ASSERT_EQUAL(sentInfo.Tokens[tokenNum].OriginalOffset, origOffset); \
    UNIT_ASSERT_EQUAL(sentInfo.Tokens[tokenNum].Punctuation, TUtf16String::FromUtf8(text)); \
    UNIT_ASSERT_EQUAL(sentInfo.Tokens[tokenNum].SpaceBefore, spaceBefore);

Y_UNIT_TEST_SUITE(Tokenizer) {

    TOKENIZER_TEST(LineEnd, "One.\n Two.\nThree.", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 3);
        ASSERT_SENT(ss[0], 0, 6, "One.\n ", 2);
        ASSERT_SENT(ss[1], 6, 11, "Two.\n", 2);
        ASSERT_SENT(ss[2], 11, 17, "Three.", 2);
    }

    TOKENIZER_TEST(Simple1, "One two three.", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 14, "One two three.", 4);

        ASSERT_WORD(sentInfo, 0, 0, "One", false);
        ASSERT_WORD(sentInfo, 1, 4, "two", true);
        ASSERT_WORD(sentInfo, 2, 8, "three", true);
        ASSERT_PUNCT(sentInfo, 3, 13, ".", false)
    }

    TOKENIZER_TEST(Simple2, "One two three", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 13, "One two three", 3);

        ASSERT_WORD(sentInfo, 0, 0, "One", false);
        ASSERT_WORD(sentInfo, 1, 4, "two", true);
        ASSERT_WORD(sentInfo, 2, 8, "three", true);
    }

    TOKENIZER_TEST(Dot1, "st.7", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 4, "st.7", 3);

        ASSERT_WORD(sentInfo, 0, 0, "st", false);
        ASSERT_PUNCT(sentInfo, 1, 2, ".", false);
        ASSERT_WORD(sentInfo, 2, 3, "7", false);
    }

    TOKENIZER_TEST(Dot2, "st. 7", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 5, "st. 7", 3);

        ASSERT_WORD(sentInfo, 0, 0, "st", false);
        ASSERT_PUNCT(sentInfo, 1, 2, ".", false);
        ASSERT_WORD(sentInfo, 2, 4, "7", true);
    }

    TOKENIZER_TEST(Dot3, "st .7", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 5, "st .7", 3);

        ASSERT_WORD(sentInfo, 0, 0, "st", false);
        ASSERT_PUNCT(sentInfo, 1, 3, ".", true)
        ASSERT_WORD(sentInfo, 2, 4, "7", false);
    }

    TOKENIZER_TEST(Dot4, "st.7.  ", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 7, "st.7.  ", 4);

        ASSERT_WORD(sentInfo, 0, 0, "st", false);
        ASSERT_PUNCT(sentInfo, 1, 2, ".", false);
        ASSERT_WORD(sentInfo, 2, 3, "7", false);
        ASSERT_PUNCT(sentInfo, 3, 4, ".", false)
    }

    TOKENIZER_TEST(Dot5, "8634-73-44. ", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 12, "8634-73-44. ", 2);

        ASSERT_WORD(sentInfo, 0, 0, "8634-73-44", false);
        ASSERT_PUNCT(sentInfo, 1, 10, ".", false)
    }

    TOKENIZER_TEST(Split1, "st.Sankt-Peterburg", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 18, "st.Sankt-Peterburg", 3);

        ASSERT_WORD(sentInfo, 0, 0, "st", false);
        ASSERT_PUNCT(sentInfo, 1, 2, ".", false);
        ASSERT_WORD(sentInfo, 2, 3, "Sankt-Peterburg", false);
    }

    TOKENIZER_TEST(Split2, "g.Rostov'na'Donu", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 16, "g.Rostov'na'Donu", 3);

        ASSERT_WORD(sentInfo, 0, 0, "g", false);
        ASSERT_PUNCT(sentInfo, 1, 1, ".", false);
        ASSERT_WORD(sentInfo, 2, 2, "Rostov'na'Donu", false);
    }

    TOKENIZER_TEST(Split3, "g-d.Rostov'na_Donu", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 18, "g-d.Rostov'na_Donu", 3);

        ASSERT_WORD(sentInfo, 0, 0, "g-d", false);
        ASSERT_PUNCT(sentInfo, 1, 3, ".", false);
        ASSERT_WORD(sentInfo, 2, 4, "Rostov'na_Donu", false);
    }

    TOKENIZER_TEST(Split4, "A.A.Petrov", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 10, "A.A.Petrov", 5);

        ASSERT_WORD(sentInfo, 0, 0, "A", false);
        ASSERT_PUNCT(sentInfo, 1, 1, ".", false);
        ASSERT_WORD(sentInfo, 2, 2, "A", false);
        ASSERT_PUNCT(sentInfo, 3, 3, ".", false);
        ASSERT_WORD(sentInfo, 4, 4, "Petrov", false);
    }

    TOKENIZER_TEST(Split5, "4.Petrov", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 8, "4.Petrov", 3);

        ASSERT_WORD(sentInfo, 0, 0, "4", false);
        ASSERT_PUNCT(sentInfo, 1, 1, ".", false);
        ASSERT_WORD(sentInfo, 2, 2, "Petrov", false);
    }

    TOKENIZER_TEST(NoSplit1, "54.567", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        UNIT_ASSERT_EQUAL(ss.back().Tokens.size(), 1);
    }

    TOKENIZER_TEST(NoSplit2, "RT54.MS", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        UNIT_ASSERT_EQUAL(ss.back().Tokens.size(), 1);
    }

    TOKENIZER_TEST(NoSplit3, "yandex.ru", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        UNIT_ASSERT_EQUAL(ss.back().Tokens.size(), 1);
    }

    TOKENIZER_TEST(NoSplit4, "888.ru", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        UNIT_ASSERT_EQUAL(ss.back().Tokens.size(), 1);
    }

    TOKENIZER_TEST(AllSplit1, "RT54.MS", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 7, "RT54.MS", 4);

        ASSERT_WORD(sentInfo, 0, 0, "RT", false);
        ASSERT_WORD(sentInfo, 1, 2, "54", false);
        ASSERT_PUNCT(sentInfo, 2, 4, ".", false);
        ASSERT_WORD(sentInfo, 3, 5, "MS", false);
    }

    TOKENIZER_TEST(AllSplit2, "A.A.Petrov", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 10, "A.A.Petrov", 5);

        ASSERT_WORD(sentInfo, 0, 0, "A", false);
        ASSERT_PUNCT(sentInfo, 1, 1, ".", false)
        ASSERT_WORD(sentInfo, 2, 2, "A", false);
        ASSERT_PUNCT(sentInfo, 3, 3, ".", false)
        ASSERT_WORD(sentInfo, 4, 4, "Petrov", false);
    }

    TOKENIZER_TEST(AllSplitPrefix, "$346", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 4, "$346", 2);

        ASSERT_PUNCT(sentInfo, 0, 0, "$", false);
        ASSERT_WORD(sentInfo, 1, 1, "346", false);
    }

    TOKENIZER_TEST(AllSplitSuffix, "aaa#", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 4, "aaa#", 2);

        ASSERT_WORD(sentInfo, 0, 0, "aaa", false);
        ASSERT_PUNCT(sentInfo, 1, 3, "#", false);
    }

    TOKENIZER_TEST(AllSplitMultitokenPrefix, "$34.0", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 5, "$34.0", 4);

        ASSERT_PUNCT(sentInfo, 0, 0, "$", false);
        ASSERT_WORD(sentInfo, 1, 1, "34", false);
        ASSERT_PUNCT(sentInfo, 2, 3, ".", false);
        ASSERT_WORD(sentInfo, 3, 4, "0", false);
    }

    TOKENIZER_TEST(AllSplitMultitokenAffix, "8+$D", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 4, "8+$D", 4);

        ASSERT_WORD(sentInfo, 0, 0, "8", false);
        ASSERT_PUNCT(sentInfo, 1, 1, "+", false);
        ASSERT_PUNCT(sentInfo, 2, 2, "$", false);
        ASSERT_WORD(sentInfo, 3, 3, "D", false);
    }

    TOKENIZER_TEST(Delim1, "word - +76", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 10, "word - +76", 4);

        ASSERT_PUNCT(sentInfo, 1, 5, "-", true);
        ASSERT_PUNCT(sentInfo, 2, 7, "+", true);
    }

    TOKENIZER_TEST(Delim2, "\"word\", \"word\"", TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 14, "\"word\", \"word\"", 7);

        ASSERT_PUNCT(sentInfo, 0, 0, "\"", false);
        ASSERT_PUNCT(sentInfo, 2, 5, "\"", false);
        ASSERT_PUNCT(sentInfo, 3, 6, ",", false);
        ASSERT_PUNCT(sentInfo, 4, 8, "\"", true);
        ASSERT_PUNCT(sentInfo, 6, 13, "\"", false);
    }

    TOKENIZER_TEST(MaxTokens, "test test test test test test test. Test test",
        TTokenizeOptions(BD_NONE, MS_MINIMAL, 6)) {

        UNIT_ASSERT_EQUAL(ss.size(), 3);

        ASSERT_SENT(ss[0], 0, 29, "test test test test test test", 6);
        ASSERT_SENT(ss[1], 29, 36, " test. ", 2);
        ASSERT_SENT(ss[2], 36, 45, "Test test", 2);
    }

    TOKENIZER_TEST(BDNone, "test test\ntest test\n\ttest",
        TTokenizeOptions(BD_NONE)) {

        UNIT_ASSERT_EQUAL(ss.size(), 1);
        ASSERT_SENT(ss.front(), 0, 25, "test test\ntest test\n\ttest", 5);
    }

    TOKENIZER_TEST(BDPerLine, "test0\ntest1\n\ntest2.\n\ttest3",
        TTokenizeOptions(BD_PER_LINE)) {

        UNIT_ASSERT_EQUAL(ss.size(), 4);

        ASSERT_SENT(ss[0], 0, 6, "test0\n", 1);
        ASSERT_WORD(ss[0], 0, 0, "test0", false);

        ASSERT_SENT(ss[1], 6, 12, "test1\n", 1);
        ASSERT_WORD(ss[1], 0, 0, "test1", false);

        ASSERT_SENT(ss[2], 13, 20, "test2.\n", 2);
        ASSERT_WORD(ss[2], 0, 0, "test2", false);

        ASSERT_SENT(ss[3], 20, 26, "\ttest3", 1);
        ASSERT_WORD(ss[3], 0, 1, "test3", true);
    }

    TOKENIZER_TEST(BDDefault1, "test\ntest\n test2\n- test3",
        TTokenizeOptions(BD_DEFAULT)) {

        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 24, "test\ntest\n test2\n- test3", 5);

        ASSERT_WORD(sentInfo, 0,  0, "test", false);
        ASSERT_WORD(sentInfo, 1,  5, "test", true);
        ASSERT_WORD(sentInfo, 2, 11, "test2", true);
        ASSERT_PUNCT(sentInfo, 3, 17, "-", true);
        ASSERT_WORD(sentInfo, 4, 19, "test3", true);
    }

    TOKENIZER_TEST(BDDefault2, "test0\n\ntest1\n\ttest2\n  test3\n\n\ntest4\n \ttest5\n     test6\n -test7\n\t-test8",
        TTokenizeOptions(BD_DEFAULT)) {

        UNIT_ASSERT_EQUAL(ss.size(), 9);

        ASSERT_SENT(ss[0], 0, 6, "test0\n", 1);
        ASSERT_WORD(ss[0], 0, 0, "test0", false);

        ASSERT_SENT(ss[1], 6, 13, "\ntest1\n", 1);
        ASSERT_WORD(ss[1], 0, 1, "test1", true);

        ASSERT_SENT(ss[2], 13, 20, "\ttest2\n", 1);
        ASSERT_WORD(ss[2], 0, 1, "test2", true);

        ASSERT_SENT(ss[3], 20, 28, "  test3\n", 1);
        ASSERT_WORD(ss[3], 0, 2, "test3", true);

        ASSERT_SENT(ss[4], 29, 36, "\ntest4\n", 1);
        ASSERT_WORD(ss[4], 0, 1, "test4", true);

        ASSERT_SENT(ss[5], 36, 44, " \ttest5\n", 1);
        ASSERT_WORD(ss[5], 0, 2, "test5", true);

        ASSERT_SENT(ss[6], 44, 55, "     test6\n", 1);
        ASSERT_WORD(ss[6], 0, 5, "test6", true);

        ASSERT_SENT(ss[7], 55, 63, " -test7\n", 2);
        ASSERT_PUNCT(ss[7], 0, 1, "-", true);
        ASSERT_WORD(ss[7], 1, 2, "test7", false);

        ASSERT_SENT(ss[8], 63, 70, "\t-test8", 2);
        ASSERT_PUNCT(ss[8], 0, 1, "-", true);
        ASSERT_WORD(ss[8], 1, 2, "test8", false);
    }

    TOKENIZER_TEST(BDPerLine_Offset1, "test test\r\ntest test",
        TTokenizeOptions(BD_PER_LINE)) {
        UNIT_ASSERT_EQUAL(ss.size(), 2);

        ASSERT_SENT(ss[0], 0, 11, "test test\r\n", 2);
        ASSERT_WORD(ss[0], 0, 0, "test", false);

        ASSERT_SENT(ss[1], 11, 20, "test test", 2);
        ASSERT_WORD(ss[1], 0, 0, "test", false);
    }

    TOKENIZER_TEST(BDPerLine_Offset2, "test test\r\ntest test",
        TTokenizeOptions(TF_OFFSET_PER_BLOCK, BD_PER_LINE)) {
        UNIT_ASSERT_EQUAL(ss.size(), 2);

        ASSERT_SENT(ss[0], 0, 11, "test test\r\n", 2);
        ASSERT_WORD(ss[0], 0, 0, "test", false);

        ASSERT_SENT(ss[1], 0, 9, "test test", 2);
        ASSERT_WORD(ss[1], 0, 0, "test", false);
    }

    TOKENIZER_TEST_STREAM(BDPerLine_Offset3, "test test\r\ntest test",
        TTokenizeOptions(BD_PER_LINE)) {
        UNIT_ASSERT_EQUAL(ss.size(), 2);

        ASSERT_SENT(ss[0], 0, 11, "test test\r\n", 2);
        ASSERT_WORD(ss[0], 0, 0, "test", false);

        ASSERT_SENT(ss[1], 11, 20, "test test", 2);
        ASSERT_WORD(ss[1], 0, 0, "test", false);
    }

    TOKENIZER_TEST_STREAM(BDPerLine_Offset4, "test test\r\ntest test",
        TTokenizeOptions(TF_OFFSET_PER_BLOCK, BD_PER_LINE)) {
        UNIT_ASSERT_EQUAL(ss.size(), 2);

        ASSERT_SENT(ss[0], 0, 11, "test test\r\n", 2);
        ASSERT_WORD(ss[0], 0, 0, "test", false);

        ASSERT_SENT(ss[1], 0, 9, "test test", 2);
        ASSERT_WORD(ss[1], 0, 0, "test", false);
    }

    TOKENIZER_TEST(BDDefault_Offset1, "test test\r\ntest test\r\n\r\ntest test",
        TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 2);

        ASSERT_SENT(ss[0], 0, 22, "test test\r\ntest test\r\n", 4);
        ASSERT_WORD(ss[0], 0, 0, "test", false);
        ASSERT_WORD(ss[0], 2, 11, "test", true);

        ASSERT_SENT(ss[1], 22, 33, "\r\ntest test", 2);
        ASSERT_WORD(ss[1], 0, 2, "test", true);
    }

    TOKENIZER_TEST(BDDefault_Offset2, "test test\r\ntest test\r\n\r\ntest test",
        TTokenizeOptions(TF_OFFSET_PER_BLOCK)) {
        UNIT_ASSERT_EQUAL(ss.size(), 2);

        ASSERT_SENT(ss[0], 0, 22, "test test\r\ntest test\r\n", 4);
        ASSERT_WORD(ss[0], 0, 0, "test", false);
        ASSERT_WORD(ss[0], 2, 11, "test", true);

        ASSERT_SENT(ss[1], 0, 11, "\r\ntest test", 2);
        ASSERT_WORD(ss[1], 0, 2, "test", true);
    }

    TOKENIZER_TEST_STREAM(BDDefault_Offset3, "test test\r\ntest test\r\n\r\ntest test",
        TTokenizeOptions()) {
        UNIT_ASSERT_EQUAL(ss.size(), 2);

        ASSERT_SENT(ss[0], 0, 22, "test test\r\ntest test\r\n", 4);
        ASSERT_WORD(ss[0], 0, 0, "test", false);
        ASSERT_WORD(ss[0], 2, 11, "test", true);

        ASSERT_SENT(ss[1], 22, 33, "\r\ntest test", 2);
        ASSERT_WORD(ss[1], 0, 2, "test", true);
    }

    TOKENIZER_TEST_STREAM(BDDefault_Offset4, "test test\r\ntest test\r\n\r\ntest test",
        TTokenizeOptions(TF_OFFSET_PER_BLOCK)) {

        ASSERT_SENT(ss[0], 0, 22, "test test\r\ntest test\r\n", 4);
        ASSERT_WORD(ss[0], 0, 0, "test", false);
        ASSERT_WORD(ss[0], 2, 11, "test", true);

        ASSERT_SENT(ss[1], 0, 11, "\r\ntest test", 2);
        ASSERT_WORD(ss[1], 0, 2, "test", true);
    }

    TOKENIZER_TEST(MinimalSplitWordsHyphen, "red-squirrel", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 12, "red-squirrel", 1);

        ASSERT_WORD(sentInfo, 0, 0, "red-squirrel", false);
    }

    TOKENIZER_TEST(MinimalSplitWordsApostrophe, "d'Artagnan dʼArtagnan d’Artagnan", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 32, "d'Artagnan dʼArtagnan d’Artagnan", 3);

        ASSERT_WORD(sentInfo, 0, 0, "d'Artagnan", false);
        ASSERT_WORD(sentInfo, 1, 11, "dʼArtagnan", true);
        ASSERT_WORD(sentInfo, 2, 22, "d’Artagnan", true);
    }

    TOKENIZER_TEST(MinimalSplitWordsDot, "g.Peterburg", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 11, "g.Peterburg", 3);

        ASSERT_WORD(sentInfo, 0, 0, "g", false);
        ASSERT_PUNCT(sentInfo, 1, 1, ".", false)
        ASSERT_WORD(sentInfo, 2, 2, "Peterburg", false);
    }

    TOKENIZER_TEST(MinimalSplitWordsDotNoSplit, "g.peterburg", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 11, "g.peterburg", 1);

        ASSERT_WORD(sentInfo, 0, 0, "g.peterburg", false);
    }

    TOKENIZER_TEST(MinimalSplitNumbersHyphen, "123-456", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 7, "123-456", 1);

        ASSERT_WORD(sentInfo, 0, 0, "123-456", false);
    }

    TOKENIZER_TEST(MinimalSplitNumbersApostrophe, "123'456ʼ789’0", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 13, "123'456ʼ789’0", 1);

        ASSERT_WORD(sentInfo, 0, 0, "123'456ʼ789’0", false);
    }

    TOKENIZER_TEST(MinimalSplitNumbersDot, "123.456", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 7, "123.456", 1);

        ASSERT_WORD(sentInfo, 0, 0, "123.456", false);
    }

    TOKENIZER_TEST(MinimalSplitMixedNoDelim, "squirrel123 123squirrel s123q456", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 32, "squirrel123 123squirrel s123q456", 3);

        ASSERT_WORD(sentInfo, 0, 0, "squirrel123", false);
        ASSERT_WORD(sentInfo, 1, 12, "123squirrel", true);
        ASSERT_WORD(sentInfo, 2, 24, "s123q456", true);
    }

    TOKENIZER_TEST(MinimalSplitMixedDot, "squirrel.123 123.Squirrel", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 25, "squirrel.123 123.Squirrel", 6);

        ASSERT_WORD(sentInfo, 0, 0, "squirrel", false);
        ASSERT_PUNCT(sentInfo, 1, 8, ".", false)
        ASSERT_WORD(sentInfo, 2, 9, "123", false);
        ASSERT_WORD(sentInfo, 3, 13, "123", true);
        ASSERT_PUNCT(sentInfo, 4, 16, ".", false);
        ASSERT_WORD(sentInfo, 5, 17, "Squirrel", false);
    }

    TOKENIZER_TEST(MinimalSplitMixedDotNoSplit, "123.squirrel", TTokenizeOptions(BD_DEFAULT, MS_MINIMAL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 12, "123.squirrel", 1);

        ASSERT_WORD(sentInfo, 0, 0, "123.squirrel", false);
    }

    TOKENIZER_TEST(SmartSplitWordsHyphen, "red-squirrel", TTokenizeOptions(BD_DEFAULT, MS_SMART)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 12, "red-squirrel", 1);

        ASSERT_WORD(sentInfo, 0, 0, "red-squirrel", false);
    }

    TOKENIZER_TEST(SmartSplitWordsApostrophe, "d'Artagnan dʼArtagnan d’Artagnan", TTokenizeOptions(BD_DEFAULT, MS_SMART)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 32, "d'Artagnan dʼArtagnan d’Artagnan", 3);

        ASSERT_WORD(sentInfo, 0, 0, "d'Artagnan", false);
        ASSERT_WORD(sentInfo, 1, 11, "dʼArtagnan", true);
        ASSERT_WORD(sentInfo, 2, 22, "d’Artagnan", true);
    }

    TOKENIZER_TEST(SmartSplitWordsDot, "g.Peterburg", TTokenizeOptions(BD_DEFAULT, MS_SMART)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 11, "g.Peterburg", 3);

        ASSERT_WORD(sentInfo, 0, 0, "g", false);
        ASSERT_PUNCT(sentInfo, 1, 1, ".", false)
        ASSERT_WORD(sentInfo, 2, 2, "Peterburg", false);
    }

    TOKENIZER_TEST(SmartSplitNumbersHyphen, "123-456", TTokenizeOptions(BD_DEFAULT, MS_SMART)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 7, "123-456", 3);

        ASSERT_WORD(sentInfo, 0, 0, "123", false);
        ASSERT_PUNCT(sentInfo, 1, 3, "-", false);
        ASSERT_WORD(sentInfo, 2, 4, "456", false);
    }

    TOKENIZER_TEST(SmartSplitNumbersApostrophe, "123'456ʼ789’0", TTokenizeOptions(BD_DEFAULT, MS_SMART)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 13, "123'456ʼ789’0", 7);

        ASSERT_WORD(sentInfo, 0, 0, "123", false);
        ASSERT_PUNCT(sentInfo, 1, 3, "'", false);
        ASSERT_WORD(sentInfo, 2, 4, "456", false);
        ASSERT_WORD(sentInfo, 3, 7, "ʼ", false);
        ASSERT_WORD(sentInfo, 4, 8, "789", false);
        ASSERT_PUNCT(sentInfo, 5, 11, "’", false);
        ASSERT_WORD(sentInfo, 6, 12, "0", false);
    }

    TOKENIZER_TEST(SmartSplitNumbersDot, "123.456", TTokenizeOptions(BD_DEFAULT, MS_SMART)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 7, "123.456", 3);

        ASSERT_WORD(sentInfo, 0, 0, "123", false);
        ASSERT_PUNCT(sentInfo, 1, 3, ".", false);
        ASSERT_WORD(sentInfo, 2, 4, "456", false);
    }

    TOKENIZER_TEST(SmartSplitMixedNoDelim, "squirrel123 123squirrel s123q456", TTokenizeOptions(BD_DEFAULT, MS_SMART)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 32, "squirrel123 123squirrel s123q456", 8);

        ASSERT_WORD(sentInfo, 0, 0, "squirrel", false);
        ASSERT_WORD(sentInfo, 1, 8, "123", false);
        ASSERT_WORD(sentInfo, 2, 12, "123", true);
        ASSERT_WORD(sentInfo, 3, 15, "squirrel", false);
        ASSERT_WORD(sentInfo, 4, 24, "s", true);
        ASSERT_WORD(sentInfo, 5, 25, "123", false);
        ASSERT_WORD(sentInfo, 6, 28, "q", false);
        ASSERT_WORD(sentInfo, 7, 29, "456", false);
    }

    TOKENIZER_TEST(SmartSplitMixedDot, "squirrel.123 123.squirrel", TTokenizeOptions(BD_DEFAULT, MS_SMART)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 25, "squirrel.123 123.squirrel", 6);

        ASSERT_WORD(sentInfo, 0, 0, "squirrel", false);
        ASSERT_PUNCT(sentInfo, 1, 8, ".", false)
        ASSERT_WORD(sentInfo, 2, 9, "123", false);
        ASSERT_WORD(sentInfo, 3, 13, "123", true);
        ASSERT_PUNCT(sentInfo, 4, 16, ".", false);
        ASSERT_WORD(sentInfo, 5, 17, "squirrel", false);
    }

    TOKENIZER_TEST(AllSplitWordsHyphen, "red-squirrel", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 12, "red-squirrel", 3);

        ASSERT_WORD(sentInfo, 0, 0, "red", false);
        ASSERT_PUNCT(sentInfo, 1, 3, "-", false);
        ASSERT_WORD(sentInfo, 2, 4, "squirrel", false);
    }

    TOKENIZER_TEST(AllSplitWordsApostrophe, "d'Artagnan dʼArtagnan d’Artagnan", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 32, "d'Artagnan dʼArtagnan d’Artagnan", 7);

        ASSERT_WORD(sentInfo, 0, 0, "d", false);
        ASSERT_PUNCT(sentInfo, 1, 1, "'", false);
        ASSERT_WORD(sentInfo, 2, 2, "Artagnan", false);
        ASSERT_WORD(sentInfo, 3, 11, "dʼArtagnan", true); // Alphabetic apostrophe is not a punctuation sign.
        ASSERT_WORD(sentInfo, 4, 22, "d", true);
        ASSERT_PUNCT(sentInfo, 5, 23, "’", false);
        ASSERT_WORD(sentInfo, 6, 24, "Artagnan", false);
    }

    TOKENIZER_TEST(AllSplitWordsDot, "g.Peterburg", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 11, "g.Peterburg", 3);

        ASSERT_WORD(sentInfo, 0, 0, "g", false);
        ASSERT_PUNCT(sentInfo, 1, 1, ".", false)
        ASSERT_WORD(sentInfo, 2, 2, "Peterburg", false);
    }

    TOKENIZER_TEST(AllSplitNumbersHyphen, "123-456", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 7, "123-456", 3);

        ASSERT_WORD(sentInfo, 0, 0, "123", false);
        ASSERT_PUNCT(sentInfo, 1, 3, "-", false);
        ASSERT_WORD(sentInfo, 2, 4, "456", false);
    }

    TOKENIZER_TEST(AllSplitNumbersApostrophe, "123'456ʼ789’0", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 13, "123'456ʼ789’0", 7);

        ASSERT_WORD(sentInfo, 0, 0, "123", false);
        ASSERT_PUNCT(sentInfo, 1, 3, "'", false);
        ASSERT_WORD(sentInfo, 2, 4, "456", false);
        ASSERT_WORD(sentInfo, 3, 7, "ʼ", false); // Alphabetic apostrophe is not a punctuation sign.
        ASSERT_WORD(sentInfo, 4, 8, "789", false);
        ASSERT_PUNCT(sentInfo, 5, 11, "’", false);
        ASSERT_WORD(sentInfo, 6, 12, "0", false);
    }

    TOKENIZER_TEST(AllSplitNumbersDot, "123.456", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 7, "123.456", 3);

        ASSERT_WORD(sentInfo, 0, 0, "123", false);
        ASSERT_PUNCT(sentInfo, 1, 3, ".", false);
        ASSERT_WORD(sentInfo, 2, 4, "456", false);
    }

    TOKENIZER_TEST(AllSplitMixedNoDelim, "squirrel123 123squirrel s123q456", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 32, "squirrel123 123squirrel s123q456", 8);

        ASSERT_WORD(sentInfo, 0, 0, "squirrel", false);
        ASSERT_WORD(sentInfo, 1, 8, "123", false);
        ASSERT_WORD(sentInfo, 2, 12, "123", true);
        ASSERT_WORD(sentInfo, 3, 15, "squirrel", false);
        ASSERT_WORD(sentInfo, 4, 24, "s", true);
        ASSERT_WORD(sentInfo, 5, 25, "123", false);
        ASSERT_WORD(sentInfo, 6, 28, "q", false);
        ASSERT_WORD(sentInfo, 7, 29, "456", false);
    }

    TOKENIZER_TEST(AllSplitMixedDot, "squirrel.123 123.squirrel", TTokenizeOptions(BD_DEFAULT, MS_ALL)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 25, "squirrel.123 123.squirrel", 6);

        ASSERT_WORD(sentInfo, 0, 0, "squirrel", false);
        ASSERT_PUNCT(sentInfo, 1, 8, ".", false)
        ASSERT_WORD(sentInfo, 2, 9, "123", false);
        ASSERT_WORD(sentInfo, 3, 13, "123", true);
        ASSERT_PUNCT(sentInfo, 4, 16, ".", false);
        ASSERT_WORD(sentInfo, 5, 17, "squirrel", false);
    }

    TOKENIZER_TEST(SpaceAfterDot, "tst. -7", TTokenizeOptions(BD_NONE, MS_ALL).Set(TF_NO_SENTENCE_SPLIT)) {
        UNIT_ASSERT_EQUAL(ss.size(), 1);
        const TSentenceInfo& sentInfo = ss.back();

        ASSERT_SENT(sentInfo, 0, 7, "tst. -7", 4);

        ASSERT_WORD(sentInfo, 0, 0, "tst", false);
        ASSERT_PUNCT(sentInfo, 1, 3, ".", false);
        ASSERT_PUNCT(sentInfo, 2, 5, "-", true);
        ASSERT_WORD(sentInfo, 3, 6, "7", false);
    }
}
