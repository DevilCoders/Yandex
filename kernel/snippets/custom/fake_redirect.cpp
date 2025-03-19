#include "fake_redirect.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/smartcut/clearchar.h>
#include <kernel/snippets/titles/make_title/make_title.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <library/cpp/string_utils/url/url.h>

static const TString REPLACER_NAME = "fake_redirect";

namespace NSnippets {

TFakeRedirectReplacer::TFakeRedirectReplacer(bool isFakeForRedirect)
    : IReplacer(REPLACER_NAME)
    , IsFakeForRedirect(isFakeForRedirect)
{
}


void TFakeRedirectReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& ctx = manager->GetContext();
    if (!IsFakeForRedirect || !ctx.Snip.Snips.empty())
        return;

    TString uil = NameByLanguage(ctx.Cfg.GetUILang());
    TStaticData data(ctx.DocInfos, REPLACER_NAME);

    TUtf16String desc, title;
    if (data.TryGetAttr("snippet_" + uil, desc)) {
        data.TryGetAttr("title_" + uil, title);
    } else {
        if (!data.TryGetAttr("snippet", desc))
            return;
        data.TryGetAttr("title", title);
    }
    SmartCut(desc, ctx.IH, ctx.LenCfg.GetMaxTextSpecSnipLen(), TSmartCutOptions(ctx.Cfg));
    if (desc.empty())
        return;

    TReplaceResult res;
    res.UseText(desc, REPLACER_NAME);
    if (title) {
        ClearChars(title);
        res.UseTitle(MakeTitle(title, ctx.Cfg, ctx.QueryCtx, TMakeTitleOptions(ctx.Cfg)));
    }
    manager->Commit(res, MRK_FAKE_REDIRECT);
}

};
