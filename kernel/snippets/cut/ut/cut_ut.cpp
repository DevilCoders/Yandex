
#include <kernel/qtree/richrequest/richnode.h>

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/smartcut/hilited_length.h>
#include <kernel/snippets/strhl/glue_common.h>
#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/strhl/zonedstring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSnippets {

TString DoTestCutSnippetNoQuery(const TString& utf8Text, size_t maxSymbols) {
    TUtf16String text = UTF8ToWide(utf8Text);
    TUtf16String result = CutSnippet(text, nullptr, maxSymbols);
    return WideToUTF8(result);
}

TString DoTestCutSnippet(const TString& utf8Text, const TString& utf8Query, size_t maxSymbols) {
    TUtf16String text = UTF8ToWide(utf8Text);
    TRichTreePtr tree = CreateRichTree(UTF8ToWide(utf8Query), TCreateTreeOptions(LANG_LAT));
    TUtf16String result = CutSnippet(text, tree->Root.Get(), maxSymbols);
    return WideToUTF8(result);
}

TPassageReplyData DoTestFillReadMoreLineInReply(const TString& utf8Text, const TString& utf8ExtText,
    const TString& exps, size_t fragmentCnt)
{
    TConfigParams params;
    params.AppendExps.push_back(exps);
    TConfig cfg(params);
    TInlineHighlighter ih;
    TZonedString zonedHeadline;
    TVector<TZonedString> snipVec;
    TVector<TZonedString> extendedSnipZonedVec;
    if (fragmentCnt > 1) {
        for (size_t i = 0; i < fragmentCnt; i++) {
            snipVec.push_back(UTF8ToWide(utf8Text));
            extendedSnipZonedVec.push_back(UTF8ToWide(utf8ExtText));
        }
    } else {
        zonedHeadline = UTF8ToWide(utf8Text);
        extendedSnipZonedVec.push_back(UTF8ToWide(utf8ExtText));
    }
    TPaintingOptions paintingOptions = TPaintingOptions::DefaultSnippetOptions();
    ih.PaintPassages(zonedHeadline, paintingOptions);
    ih.PaintPassages(snipVec, paintingOptions);
    ih.PaintPassages(extendedSnipZonedVec, paintingOptions);
    TVector<TZonedString> paintedFragments;
    if (zonedHeadline.String) {
        paintedFragments.push_back(zonedHeadline);
    }
    for (const TZonedString& passage : snipVec) {
        paintedFragments.push_back(passage);
    }
    TPassageReplyData replyData;
    replyData.Passages = MergedGlue(snipVec);
    replyData.Headline = MergedGlue(zonedHeadline);
    replyData.SnipLengthInSymbols = GetHilitedTextLengthInSymbols(paintedFragments);
    replyData.SnipLengthInRows = GetHilitedTextLengthInRows(paintedFragments, cfg.GetYandexWidth(), cfg.GetSnipFontSize());

    TVector<TUtf16String> extendedSnipVec;
    for (const auto& zonedStr : extendedSnipZonedVec) {
        extendedSnipVec.push_back(zonedStr.String);
    }

    TPassageReplyData originalReplyData(replyData);
    FillReadMoreLineInReply(replyData, paintedFragments, extendedSnipVec, ih, paintingOptions, cfg, nullptr);

    UNIT_ASSERT(replyData.Headline.StartsWith(StripStringRight(originalReplyData.Headline, EqualsStripAdapter('.'))));
    UNIT_ASSERT(replyData.SnipLengthInSymbols >= originalReplyData.SnipLengthInSymbols);
    UNIT_ASSERT(replyData.SnipLengthInRows > originalReplyData.SnipLengthInRows - 0.001);
    if (fragmentCnt == 1) {
        UNIT_ASSERT_EQUAL(replyData.Headline.size(), replyData.SnipLengthInSymbols);
    }
    return replyData;
}

Y_UNIT_TEST_SUITE(TCutTests) {
    Y_UNIT_TEST(TestCutSnippet) {
        TString lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
            "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
            "enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut "
            "aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit "
            "in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur "
            "sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
            "mollit anim id est laborum.";

        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippetNoQuery(lorem, 7), "");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippetNoQuery(lorem, 8), "Lorem...");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippetNoQuery(lorem, 1000), lorem);
        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippetNoQuery(lorem, 54),
            "Lorem ipsum dolor sit amet, consectetur adipiscing...");

        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippet(lorem, "cillum fugiat", 30),
            "...cillum dolore eu fugiat...");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippet(lorem, "dolore", 50),
            "...velit esse cillum dolore eu fugiat nulla...");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippet(lorem, "ut", 40),
            "Ut enim ad minim veniam, quis nostrud...");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippet(lorem, "est laborum", 40),
            "...mollit anim id est laborum.");

        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippetNoQuery("Abc def.", 20), "Abc def.");
        UNIT_ASSERT_STRINGS_EQUAL(DoTestCutSnippetNoQuery("Abc def", 20), "Abc def...");

        TPassageReplyData replyData;
        TString cutLorem = DoTestCutSnippetNoQuery(lorem, 150);
        replyData = DoTestFillReadMoreLineInReply(cutLorem, lorem, "yandex_width=400,snipfont=12,uil_ru,maximize_len_when_fill_read_more_line=0", 1);
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(replyData.Headline), cutLorem);
        replyData = DoTestFillReadMoreLineInReply(cutLorem, lorem, "yandex_width=400,snipfont=14,uil_ru,maximize_len_when_fill_read_more_line=0", 1);
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(replyData.Headline), cutLorem);
        replyData = DoTestFillReadMoreLineInReply(cutLorem, lorem, "yandex_width=400,snipfont=14,uil_ru,maximize_len_when_fill_read_more_line=1", 1);
        UNIT_ASSERT_EQUAL(replyData.SnipLengthInSymbols, 164);
        replyData = DoTestFillReadMoreLineInReply(cutLorem, lorem, "yandex_width=400,snipfont=14,uil_ru,maximize_len_when_fill_read_more_line=1", 2);
        UNIT_ASSERT_EQUAL(replyData.SnipLengthInSymbols, 292);
        TString shortText = "Короткий.";
        TString extShortText = "Короткий. Короткий текст.";
        replyData = DoTestFillReadMoreLineInReply(shortText, extShortText, "yandex_width=128,snipfont=14,uil_ru,maximize_len_when_fill_read_more_line=0", 1);
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(replyData.Headline), shortText);
        replyData = DoTestFillReadMoreLineInReply(shortText, extShortText, "yandex_width=220,snipfont=14,uil_ru,maximize_len_when_fill_read_more_line=0", 3);
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(replyData.Passages.back()), shortText);
        replyData = DoTestFillReadMoreLineInReply(shortText, extShortText, "yandex_width=220,snipfont=14,uil_ru,maximize_len_when_fill_read_more_line=0", 4);
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(replyData.Passages.back()), extShortText);
        replyData = DoTestFillReadMoreLineInReply(shortText, extShortText, "yandex_width=220,snipfont=14,uil_kaz,maximize_len_when_fill_read_more_line=0", 4);
        UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(replyData.Passages.back()), shortText);
    }
}

}
