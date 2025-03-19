#include "delink.h"

#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>

#include <kernel/snippets/config/config.h>

namespace NSnippets {

static bool CanKillByLinkSnip(const TQueryy& query, const TConfig& cfg, const TString& url, const TSnip& snip, const TSnipTitle& title, const TUtf16String& meta) {
    return
        !cfg.ExpFlagOff("skip_nps_with_headline") ||
        !snip.Snips.empty() &&
        TSimpleSnipCmp(query, cfg).Add(title).Add(meta).AddUrl(url).Add(snip) ==
        TSimpleSnipCmp(query, cfg).Add(title).Add(meta).AddUrl(url);
}

bool CanKillByLinkSnip(const TReplaceContext& ctx) {
    return ctx.IsByLink && CanKillByLinkSnip(ctx.QueryCtx, ctx.Cfg, ctx.Url, ctx.Snip, ctx.SuperNaturalTitle, ctx.MetaDescr.GetTextCopy());
}


}
