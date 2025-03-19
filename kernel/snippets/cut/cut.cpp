#include "cut.h"

#include <kernel/snippets/algo/one_span.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/passagereply.h>
#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/restr.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/smartcut/consts.h>
#include <kernel/snippets/smartcut/cutparam.h>
#include <kernel/snippets/smartcut/hilited_length.h>
#include <kernel/snippets/smartcut/smartcut.h>
#include <kernel/snippets/strhl/glue_common.h>
#include <kernel/snippets/strhl/goodwrds.h>
#include <kernel/snippets/strhl/zonedstring.h>
#include <kernel/snippets/uni_span_iter/uni_span_iter.h>
#include <kernel/snippets/weight/weighter.h>

#include <util/system/yassert.h>

namespace NSnippets {
    namespace {
        constexpr wchar16 HILIGHT_MARK = 0x07;
    }

TSmartCutOptions::TSmartCutOptions() {
}

TSmartCutOptions::TSmartCutOptions(const TConfig& cfg) {
    StopWordsFilter = &cfg.GetStopWordsFilter();
    if (cfg.GetSnipCutMethod() == TCM_PIXEL) {
        CutParams = TCutParams::Pixel(cfg.GetYandexWidth(), cfg.GetSnipFontSize());
    }

    if (!cfg.ExpFlagOff("fix_hilighter")) {
        HilightMark = &HILIGHT_MARK;
    }
}

TMultiCutResult SmartCutWithExt(
    const TUtf16String& text,
    const TInlineHighlighter& ih,
    float maxLen,
    float maxExtLen,
    const TSmartCutOptions& options)
{
    TTextCuttingOptions textOptions;
    textOptions.StopWordsFilter = options.StopWordsFilter;
    textOptions.MaximizeLen = options.MaximizeLen;
    textOptions.Threshold = options.Threshold;
    textOptions.AddEllipsisToShortText = options.AddEllipsisToShortText;
    textOptions.CutLastWord = options.CutLastWord;
    textOptions.HilightMark = options.HilightMark;
    return SmartCutExtSnipWithQuery(text, ih, maxLen, maxExtLen, options.CutParams, textOptions);
}

void SmartCut(TUtf16String& text, const TInlineHighlighter& ih, float maxLen, const TSmartCutOptions& options) {
    text = SmartCutWithExt(text, ih, maxLen, 0.0, options).Short;
}

TUtf16String CutSnip(const TUtf16String& raw, const TConfig& cfg, const TQueryy& query,
        float maxLen, float maxLenForLPFactors, const TSnipTitle* title,
        const TWordSpanLen& wordSpanLen) {
    TRetainedSentsMatchInfo customSents;
    customSents.SetView(raw, TRetainedSentsMatchInfo::TParams(cfg, query).SetPutDot());
    const TSentsMatchInfo& sentsMatchInfo = *customSents.GetSentsMatchInfo();
    TNoRestr restr;

    TUniSpanIter wordSpan(sentsMatchInfo, restr, restr, wordSpanLen);
    TMxNetWeighter w(sentsMatchInfo, cfg, wordSpanLen, title, maxLenForLPFactors);

    const TSnip snip = NSnipWordSpans::GetTSnippet(w, wordSpan, sentsMatchInfo,
        maxLen, CS_TEXT_ARC, nullptr, nullptr, nullptr);
    return snip.GetRawTextWithEllipsis();
}

TMultiCutResult CutSnipWithExt(const TUtf16String& raw, const TReplaceContext& ctx, float maxLen, float maxExtLen,
        float maxLenForLPFactors, const TSnipTitle* title, const TWordSpanLen& wordSpanLen)
{
    TUtf16String shortStr = CutSnip(raw, ctx.Cfg, ctx.QueryCtx, maxLen, maxLenForLPFactors, title, wordSpanLen);
    TWtringBuf stripped(shortStr);
    stripped.ChopSuffix(BOUNDARY_ELLIPSIS);
    size_t startIndex = raw.find(stripped);

    if (startIndex == TUtf16String::npos) {
        return TMultiCutResult(shortStr);
    }
    TUtf16String longStr = raw.substr(startIndex);
    SmartCut(longStr, ctx.IH, maxExtLen, TSmartCutOptions(ctx.Cfg));
    return TMultiCutResult(shortStr, longStr);
}

TUtf16String CutSnip(const TUtf16String& raw, const TConfig& cfg, const TQueryy& query, float maxLen) {
    TLengthChooser lenCfg(cfg, query);
    TCutParams cutParams = (cfg.GetSnipCutMethod() == TCM_SYMBOL) ?
        TCutParams::Symbol() :
        TCutParams::Pixel(cfg.GetYandexWidth(), cfg.GetSnipFontSize());
    TWordSpanLen wordSpanLen(cutParams);
    return CutSnip(raw, cfg, query, maxLen, lenCfg.GetMaxSnipLen(), nullptr, wordSpanLen);
}

TUtf16String CutSnippet(const TUtf16String& text, const TRichRequestNode* richtreeRoot, size_t maxSymbols) {
    TConfig cfg;
    TQueryy query(richtreeRoot, cfg);
    TLengthChooser lenCfg(cfg, query);
    TWordSpanLen wordSpanLen(TCutParams::Symbol());
    TUtf16String cut = CutSnip(text, cfg, query, maxSymbols, lenCfg.GetMaxSnipLen(), nullptr, wordSpanLen);
    if (!cut) {
        TTextCuttingOptions options;
        options.AddEllipsisToShortText = true;
        cut = SmartCutSymbol(text, maxSymbols, options);
    }
    return cut;
}

static THashMap<ELanguage, TUtf16String> READ_MORE_TEXTS = {
    {LANG_RUS, u" Читать ещё >"},
    {LANG_KAZ, u" Тағы оқу >"},
    {LANG_BEL, u" Чытаць яшчэ >"},
    {LANG_UKR, u" Читати ще >"},
    {LANG_TAT, u" Тагын уку >"},
    {LANG_TUR, u" Devamını oku >"},
    {LANG_ENG, u" Read more >"},
};

static const TUtf16String READ_MORE_TEXT_DEFAULT = u" Читать ещё >";

static const TUtf16String& GetReadMoreText(ELanguage uiLang) {
    if (READ_MORE_TEXTS.contains(uiLang)) {
        return READ_MORE_TEXTS[uiLang];
    } else {
        return READ_MORE_TEXT_DEFAULT;
    }
}

void FillReadMoreLineInReply(TPassageReplyData& replyData, TVector<TZonedString>& paintedFragments,
    const TVector<TUtf16String>& extendedSnipVec,
    const TInlineHighlighter& highlighter, const TPaintingOptions& paintingOptions,
    const TConfig& cfg, ISnippetsCallback* explainCallback)
{
    Y_ASSERT(cfg.GetSnipCutMethod() == TCM_PIXEL);
    Y_ASSERT(!!paintedFragments);
    // +paintedFragments < +extendedSnipVec is possible, skip this case for now
    if (cfg.GetSnipCutMethod() != TCM_PIXEL || !paintedFragments || !extendedSnipVec || paintedFragments.size() != extendedSnipVec.size()) {
        return;
    }
    const TUtf16String& readMoreText = GetReadMoreText(cfg.GetUILang());
    TVector<TBoldSpan> boldSpans;
    TPixelLengthCalculator calculator(readMoreText, boldSpans);
    float readMoreLen = calculator.CalcLengthInRows(0, readMoreText.size(), cfg.GetSnipFontSize(), cfg.GetYandexWidth());
    float freeSpaceLen = ceil(replyData.SnipLengthInRows) - replyData.SnipLengthInRows;
    if (freeSpaceLen > readMoreLen || readMoreLen > 0.5) {
        // If "read more" adds a line and snip width is not too low, try extending the snippet, else return.
        return;
    }
    TVector<TZonedString> lastFragment;
    lastFragment.push_back(paintedFragments.back());
    float lastFragmentLen = GetHilitedTextLengthInRows(lastFragment, cfg.GetYandexWidth(), cfg.GetSnipFontSize());
    float lastFragmentMaxLen = lastFragmentLen + 1.0 - readMoreLen + freeSpaceLen;
    TSmartCutOptions cutOptions(cfg);
    if (!cfg.ExpFlagOff("maximize_len_when_fill_read_more_line")) {
        cutOptions.MaximizeLen = true;
    }
    TUtf16String lastFragmentExtString = extendedSnipVec.back();
    SmartCut(lastFragmentExtString, highlighter, lastFragmentMaxLen, cutOptions);

    if (lastFragmentExtString.size() <= lastFragment.back().String.size()) {
        // extension result is not longer than source snippet
        return;
    }
    if (explainCallback != nullptr && explainCallback->GetDebugOutput()) {
        explainCallback->GetDebugOutput()->Print(false, "Snippet was extended to fill empty read more line");
    }
    paintedFragments.back() = lastFragmentExtString;
    highlighter.PaintPassages(paintedFragments.back(), paintingOptions);

    if (!!replyData.Passages) {
        if (!!paintedFragments.back().String) {
            replyData.Passages.back() = MergedGlue(paintedFragments.back());
        } else {
            replyData.Passages.pop_back();
        }
    } else if (!!replyData.Headline) {
        replyData.Headline = MergedGlue(paintedFragments.back());
    }
    replyData.SnipLengthInSymbols = GetHilitedTextLengthInSymbols(paintedFragments);
    replyData.SnipLengthInRows = GetHilitedTextLengthInRows(paintedFragments, cfg.GetYandexWidth(), cfg.GetSnipFontSize());
}

} // namespace NSnippets
