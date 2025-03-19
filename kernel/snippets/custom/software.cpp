#include "software.h"
#include "extended_length.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/read_helper/read_helper.h>
#include <kernel/snippets/schemaorg/software.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/string_utils/url/url.h>

namespace NSnippets {

static const char* const HEADLINE_SRC = "software";
static const size_t MIN_SNIP_LEN = 120;

static const TStringBuf WHITELISTED_HOSTS[] = {
    "play.google.com",
    "itunes.apple.com",
};

static bool IsWhitelisted(const TStringBuf& url) {
    TStringBuf host = GetOnlyHost(url);
    for (const TStringBuf& wlHost : WHITELISTED_HOSTS) {
        if (host == wlHost) {
            return true;
        }
    }
    return false;
}

static bool IsNotReadable(const TReplaceContext& ctx, const TUtf16String& text) {
    TReadabilityChecker checker(ctx.Cfg, ctx.QueryCtx, ctx.DocLangId);
    checker.CheckShortSentences = true;
    checker.CheckBadCharacters = true;
    checker.CheckLanguageMatch = true;
    checker.CheckCapitalization = true;
    checker.CheckRepeatedWords = true;
    return !checker.IsReadable(text, false);
}

TSoftwareApplicationReplacer::TSoftwareApplicationReplacer(const TSchemaOrgArchiveViewer& arcViewer)
    : IReplacer("software")
    , ArcViewer(arcViewer)
{
}

void TSoftwareApplicationReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& ctx = manager->GetContext();

    const NSchemaOrg::TSoftwareApplication* app = ArcViewer.GetSoftwareApplication();
    if (!app) {
        return;
    }
    if (ctx.Cfg.IsMainPage()) {
        manager->ReplacerDebug("the main page is not processed");
        return;
    }
    TUtf16String text = app->FormatSnip(GetOnlyHost(ctx.Url), ctx.DocLangId);
    if (!text) {
        manager->ReplacerDebug("SoftwareApplication/description field is missing");
        return;
    }
    float maxLen = ctx.LenCfg.GetMaxTextSpecSnipLen();
    float maxExtLen = GetExtSnipLen(ctx.Cfg, ctx.LenCfg);
    const auto& extSnip = SmartCutWithExt(text, ctx.IH, maxLen, maxExtLen, TSmartCutOptions(ctx.Cfg));
    TReplaceResult res;
    res.UseText(extSnip, HEADLINE_SRC);
    if (extSnip.Short.size() < MIN_SNIP_LEN) {
        manager->ReplacerDebug("headline is too short", res);
        return;
    }
    if (IsNotReadable(ctx, extSnip.Short)) {
        manager->ReplacerDebug("headline is not readable", res);
        return;
    }
    if (!IsWhitelisted(ctx.Url) &&
        TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(ctx.SuperNaturalTitle).Add(ctx.Snip) >
        TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(ctx.NaturalTitle).Add(extSnip.Short))
    {
        manager->ReplacerDebug("headline has not enough query words", res);
        return;
    }
    manager->Commit(res, MRK_SOFTWARE);
}

} // namespace NSnippets
