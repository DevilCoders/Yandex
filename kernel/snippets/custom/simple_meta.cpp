#include "simple_meta.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/delink/delink.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <util/generic/string.h>


namespace NSnippets {
    void TMetaDescrReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        if (!ctx.Snip.Snips.empty() && !ctx.IsByLink) {
            return;
        }
        TUtf16String meta = ctx.MetaDescr.GetTextCopy();
        if (!meta) {
            meta = ctx.MetaDescr.GetLowQualityText();
        }
        if (!meta) {
            return;
        }
        TSmartCutOptions options(ctx.Cfg);
        options.MaximizeLen = true;
        SmartCut(meta, ctx.IH, ctx.LenCfg.GetMaxSnipLen(), options);
        TReplaceResult res;
        if (ctx.IsByLink && !CanKillByLinkSnip(ctx)) {
            res.SetPreserveSnip();
        }
        res.UseText(meta, "meta_descr");
        res.UseTitle(manager->GetContext().SuperNaturalTitle);
        manager->Commit(res, MRK_META_DESCR);
    }
}

