#include "mediawiki.h"
#include "extended_length.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/delink/delink.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/extra_attrs.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/single_snip.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <kernel/lemmer/alpha/abc.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <library/cpp/langs/langs.h>
#include <util/charset/unidata.h>
#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/subst.h>
#include <library/cpp/string_utils/url/url.h>
#include <utility>

namespace NSnippets {
namespace {

const char* const HEADLINE_SRC = "mediawiki_snip";
const size_t MIN_HEADLINE_LEN = 50;
const size_t MIN_LISTEN_LEN = 40;
const size_t MAX_LISTEN_LEN = 250;
const float MIN_EXTEND_RATIO = 1.3;

const TUtf16String SUBSTRS_TO_CUT[] = {
    u"[edit]",
    u"[править]",
    u"[править | править вики-текст]",
    u"[ред. • ред. код]",
    u"[değiştir | kaynağı değiştir]",
    u"[citation needed]",
    u"[привести цитату]",
    u"(англ.)русск.",
};

TUtf16String GetSingleSnipWithoutAux(const TSingleSnip& ssnip) {
    TUtf16String text = ToWtring(ssnip.GetTextBuf());
    for (const TUtf16String& cut : SUBSTRS_TO_CUT) {
        SubstGlobal(text, cut, TUtf16String());
    }
    Strip(text);
    Collapse(text);
    return text;
}

void CutParentheses(TUtf16String& text) {
    size_t openPos = 0;
    while ((openPos = text.find('(')) != text.npos) {
        size_t closePos = openPos + 1;
        int balance = 1;
        for (; closePos + 1 < text.size(); ++closePos) {
            if (text[closePos] == '(') {
                ++balance;
            } else if (text[closePos] == ')') {
                --balance;
                if (balance == 0) {
                    break;
                }
            }
        }
        while (openPos > 0 && IsWhitespace(text[openPos - 1])) {
            --openPos;
        }
        text.erase(openPos, closePos - openPos + 1);
    }
}

const wchar16 LEFT_TO_RIGHT_MARK = wchar16(0x200E);
const wchar16 RIGHT_TO_LEFT_MARK = wchar16(0x200F);
const wchar16 MIN_HEBREW_CHAR = wchar16(0x0590);
const wchar16 MAX_HEBREW_CHAR = wchar16(0x05FF);

bool IsNonCyrillicChar(wchar16 c) {
    if (c == LEFT_TO_RIGHT_MARK || c == RIGHT_TO_LEFT_MARK) {
        return true;
    }
    if (MIN_HEBREW_CHAR <= c && c <= MAX_HEBREW_CHAR) {
        return true;
    }
    TLangMask langs = NLemmer::GetCharInfoAlpha(c);
    return !langs.Empty() && !langs.SafeTest(LANG_RUS);
}

class TSentencesHandler : public ITokenHandler {
public:
    size_t FullSentencesEnd = 0;
    size_t Position = 0;
    bool Finished = false;
public:
    void OnToken(const TWideToken& token, size_t origleng, NLP_TYPE type) override {
        if (Finished) {
            return;
        }
        TWtringBuf tok = token.Text();
        for (wchar16 c : tok) {
            if (IsNonCyrillicChar(c)) {
                Finished = true;
                return;
            }
        }
        Position += origleng;
        if (Position > MAX_LISTEN_LEN) {
            Finished = true;
            return;
        }
        if (type == NLP_SENTBREAK || type == NLP_PARABREAK || type == NLP_END) {
            if (!tok.StartsWith(u"...")) {
                FullSentencesEnd = Position;
            }
        }
    }
};

void CutPartialOrNonCyrillicSentences(TUtf16String& text) {
    text += u" Flush"; // to force last sentbreak event
    TSentencesHandler handler;
    TNlpTokenizer(handler).Tokenize(text);
    text.erase(handler.FullSentencesEnd);
    while (text && IsWhitespace(text.back())) {
        text.pop_back();
    }
}

const wchar16 COMBINING_ACUTE_ACCENT = wchar16(769);

TUtf16String GetListenSummary(const TUtf16String& articleSummary) {
    TUtf16String text = articleSummary;
    CutParentheses(text);
    SubstGlobal(text, TUtf16String(COMBINING_ACUTE_ACCENT), TUtf16String());
    CutPartialOrNonCyrillicSentences(text);
    if (text && text.size() < MIN_LISTEN_LEN) {
        text.clear();
    }
    return text;
}

static TStringBuf LISTEN_SUMMARY_PREFIX = "По данным Википедии, ";
static char16_t TITLE_DELIMITER = L'—';

class TMediawikiData {
public:
    TUtf16String ArticleTitle;
    TUtf16String ArticleSummary;
    TUtf16String ArticleSummaryFirstPara;
public:
    explicit TMediawikiData(const TReplaceContext& ctx) {
        TStaticData data(ctx.DocInfos, "mediawiki");
        ArticleTitle = data.Attrs["title"];
        if (!ArticleTitle) {
            UpdateArticleTitle(ctx.NaturalTitle.GetTitleString());
        }
        if (!ArticleTitle) {
            UpdateArticleTitle(ctx.SuperNaturalTitle.GetTitleString());
        }
        const TUtf16String& desc = data.Attrs["desc"];
        ArticleSummary = desc;
        SubstGlobal(ArticleSummary, MEDIA_WIKI_PARA_MARKER, TUtf16String());
        size_t pos = desc.find(MEDIA_WIKI_PARA_MARKER);
        ArticleSummaryFirstPara = desc.substr(0, pos == desc.npos ? desc.size() : pos);
    }
private:
    void UpdateArticleTitle(const TUtf16String& title) {
        TWtringBuf titleBuf(title);
        TWtringBuf tok;
        if (titleBuf.RNextTok(TITLE_DELIMITER, tok) && !titleBuf.empty()) {
            ArticleTitle = Strip(titleBuf);
        }
    }
};

void AddExtraSnipAttrs(TReplaceManager* manager, const TMediawikiData& mwData) {
    const TReplaceContext& ctx = manager->GetContext();
    NJson::TJsonValue features(NJson::JSON_MAP);
    bool hasData = false;
    if (mwData.ArticleTitle) {
        features["article_title"] = WideToUTF8(mwData.ArticleTitle);
        hasData = true;
    }
    TStringBuf host = GetOnlyHost(ctx.Url);
    if (host == "ru.wikipedia.org") {
        TUtf16String listenSummary = GetListenSummary(mwData.ArticleSummaryFirstPara);
        if (listenSummary) {
            NJson::TJsonValue voiceInfoRuItem(NJson::JSON_MAP);
            voiceInfoRuItem["text"] = LISTEN_SUMMARY_PREFIX + WideToUTF8(listenSummary);
            voiceInfoRuItem["lang"] = "ru-RU";
            NJson::TJsonValue voiceInfoRu(NJson::JSON_ARRAY);
            voiceInfoRu.AppendValue(voiceInfoRuItem);
            NJson::TJsonValue voiceInfo(NJson::JSON_MAP);
            voiceInfo["ru"] = voiceInfoRu;
            features["voiceInfo"] = voiceInfo;
            hasData = true;
        }
    }
    if (!hasData) {
        return;
    }
    features["wikipedia"] = host.EndsWith(".wikipedia.org");
    features["type"] = "mediawiki";
    NJson::TJsonValue data(NJson::JSON_MAP);
    data["content_plugin"] = true;
    data["features"] = features;
    manager->GetExtraSnipAttrs().AddClickLikeSnipJson("mediawiki", NJson::WriteJson(data));
}

void AddBigMediawikiSnipAttrs(TReplaceManager* manager, const TUtf16String& title, const TMultiCutResult& extSnip, const TString& url) {
    NJson::TJsonValue features(NJson::JSON_MAP);
    features["headline"] = WideToUTF8(title);
    features["text"] = WideToUTF8(extSnip.Short);
    if (extSnip.Long.length() > MIN_EXTEND_RATIO * extSnip.Short.length()) {
        features["extended_text"] = WideToUTF8(extSnip.Long);
    }
    features["source"] = "big_mediawiki";
    features["type"] = "suggest_fact";
    features["host"] = "ru.wikipedia.org";
    features["url"] = url;
    NJson::TJsonValue data(NJson::JSON_MAP);
    data["block_type"] = "construct";
    data["slot"] = "pre";
    data["features"] = features;
    NJson::TJsonValue allowedPositions(NJson::JSON_ARRAY);
    allowedPositions.AppendValue(0);
    data["allowed_positions"] = allowedPositions;
    manager->GetExtraSnipAttrs().AddClickLikeSnipJson("big_mediawiki", NJson::WriteJson(data));
}

bool ConvertSnip(const TSnip& snip, TCustomSnippetsStorage& customSnips, TSnip& result, const TReplaceContext& ctx) {
    if (snip.Snips.empty()) {
        return false;
    }
    if (!ctx.DocInfos.contains("mediawiki")) {
        return false;
    }
    bool updated = false;
    TVector<TUtf16String> snipTexts;
    TVector<bool> breakBefore;
    TVector<bool> breakAfter;
    for (const TSingleSnip& ssnip : snip.Snips) {
        TUtf16String text = GetSingleSnipWithoutAux(ssnip);
        if (text != ssnip.GetTextBuf()) {
            updated = true;
        }
        snipTexts.push_back(text);
        breakBefore.push_back(ssnip.BeginsWithSentBreak());
        breakAfter.push_back(ssnip.EndsWithSentBreak());
    }
    if (!updated) {
        return false;
    }
    TRetainedSentsMatchInfo& customSents = customSnips.CreateRetainedInfo();
    customSents.SetView(snipTexts, TRetainedSentsMatchInfo::TParams(ctx.Cfg, ctx.QueryCtx));
    result = customSents.AllAsSnip();
    for (TSingleSnip& ssnip : result.Snips) {
        const TSentsInfo& nsi = ssnip.GetSentsMatchInfo()->SentsInfo;
        int arcId = nsi.GetArchiveSent(ssnip.GetFirstSent()).SentId;
        if (breakBefore[arcId]) {
            ssnip.SetForceDotsAtBegin(true);
        }
        if (breakAfter[arcId]) {
            ssnip.SetForceDotsAtEnd(true);
        }
    }
    return !result.Snips.empty();
}

bool HandleMediaWikiAttr(TReplaceManager* manager) {
    const TReplaceContext& ctx = manager->GetContext();
    TMediawikiData data(ctx);
    TUtf16String text = data.ArticleSummary;
    if (!text) {
        return false;
    }
    TMultiCutResult extSnip;
    if (GetOnlyHost(ctx.Url).EndsWith(".wikipedia.org")) {
        if (!ctx.Cfg.ExpFlagOff("big_mediawiki_info")) {
            float maxFactLen = ctx.Cfg.BigMediawikiSnippetLen();
            float maxFactExtLen = !ctx.Cfg.ExpFlagOff("big_mediawiki_read_more")
                ? ctx.Cfg.BigMediawikiReadMoreLenMultiplier() * maxFactLen
                : 0.f;
            extSnip = SmartCutWithExt(text, ctx.IH, maxFactLen, maxFactExtLen, TSmartCutOptions(ctx.Cfg));
            if (data.ArticleTitle && extSnip.Short.size() >= MIN_HEADLINE_LEN &&
                ctx.Url.find(":Search?search=") == TString::npos) {
                bool shouldAddAttrs = true;
                if (!ctx.Cfg.ExpFlagOff("big_mediawiki_quality_filter")) { // SNIPPETS-3517
                    double oldWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true)
                        .Add(ctx.Snip).GetWeight();
                    double newWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true)
                        .Add(extSnip.Short).GetWeight();
                    shouldAddAttrs = (newWeight * ctx.Cfg.BigMediawikiQualityFilterMultiplier() > oldWeight - 1E-7);
                }
                if (shouldAddAttrs) {
                    AddBigMediawikiSnipAttrs(manager, data.ArticleTitle, extSnip, ctx.Url);
                }
            }
        }
    }
    if (!extSnip || !ctx.Cfg.BigMediawikiSnippet()) {
        float maxLen = ctx.LenCfg.GetMaxSpecSnipLen();
        float maxExtLen = GetExtSnipLen(ctx.Cfg, ctx.LenCfg);
        extSnip = SmartCutWithExt(text, ctx.IH, maxLen, maxExtLen, TSmartCutOptions(ctx.Cfg));
    }
    if (extSnip.Short.size() < MIN_HEADLINE_LEN) {
        return false;
    }

    TReplaceResult res;
    res.UseText(extSnip, HEADLINE_SRC);

    double oldWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true)
        .Add(ctx.SuperNaturalTitle).Add(ctx.Snip).AddUrl(ctx.Url).GetWeight();
    double newWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true)
        .Add(ctx.NaturalTitle).Add(extSnip.Short).AddUrl(ctx.Url).GetWeight();
    bool hasBetterWeight = newWeight * 2.0 >= oldWeight;

    if (hasBetterWeight || ctx.IsByLink) {
        if (ctx.IsByLink && !CanKillByLinkSnip(ctx)) {
            res.SetPreserveSnip();
        }
        AddExtraSnipAttrs(manager, data);
        manager->Commit(res, MRK_MEDIAWIKI);
        return true;
    } else {
        manager->ReplacerDebug("not enough query words in summary", res);
        return false;
    }
}

} // anonymous namespace

const TUtf16String MEDIA_WIKI_PARA_MARKER = u"\u0007\u00B6";

TMediaWikiReplacer::TMediaWikiReplacer()
    : IReplacer("mediawiki")
{
}

void TMediaWikiReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& ctx = manager->GetContext();
    if (ctx.Cfg.IsMainPage()) {
        return;
    }
    if (HandleMediaWikiAttr(manager)) {
        return;
    }
    TSnip newSnip;
    bool converted = ConvertSnip(ctx.Snip, manager->GetCustomSnippets(), newSnip, ctx);
    if (converted && !newSnip.Snips.empty()) {
        manager->Commit(TReplaceResult().UseSnip(newSnip, HEADLINE_SRC), MRK_MEDIAWIKI);
    }
}

} // namespace NSnippets
