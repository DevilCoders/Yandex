#include "kinopoisk.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>

namespace NSnippets
{
    void TKinoPoiskReplacer::DoWork(TReplaceManager* manager)
    {
        const TReplaceContext& ctx = manager->GetContext();
        TStaticData data(ctx.DocInfos, "person_card");
        const TUtf16String& title = data.Attrs["title"];
        TUtf16String desc = data.Attrs["snippet"];
        if (!title) {
            return;
        }
        TReplaceResult res;
        res.UseTitle(MakeSpecialTitle(title, ctx.Cfg, ctx.QueryCtx));
        bool text = false;
        if (desc) {
            TSmartCutOptions options(ctx.Cfg);
            options.MaximizeLen = true;
            SmartCut(desc, ctx.IH, ctx.LenCfg.GetMaxSpecSnipLen(), options);
            if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(title).Add(ctx.Snip) <=
                TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true).Add(title).Add(desc))
            {
                res.UseText(desc, "kinopoisk");
                text = true;
            }
        }
        if (!text && ctx.MetaDescr.MayUse() && (ctx.Snip.Snips.empty() || ctx.IsByLink)) {
            TUtf16String meta = ctx.MetaDescr.GetTextCopy();
            TSmartCutOptions options(ctx.Cfg);
            options.MaximizeLen = true;
            SmartCut(meta, ctx.IH, ctx.LenCfg.GetMaxSnipLen(), options);
            if (ctx.IsByLink) {
                res.SetPreserveSnip();
            }
            res.UseText(meta, "meta_descr");
        }
        manager->Commit(res, MRK_KINOPOISK);
    }
}
