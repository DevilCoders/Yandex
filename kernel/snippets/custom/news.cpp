#include "news.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>

#include <util/generic/string.h>

namespace NSnippets {
    static const size_t MIN_LENGTH = 120;

    void TNewsReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        if (!ctx.Cfg.ExpFlagOn("enable_news")) {
            return;
        }
        TStaticData data(ctx.DocInfos, "news");
        TUtf16String desc = data.Attrs["snippet"];
        if (!desc) {
            return;
        }
        if (!IsTerminal(desc.back())) {
            desc.append('.');
        }
        SmartCut(desc, ctx.IH, ctx.LenCfg.GetMaxTextSpecSnipLen(), TSmartCutOptions(ctx.Cfg));
        TReplaceResult result;
        result.UseText(desc, "news");
        if (desc.size() < MIN_LENGTH) {
            manager->ReplacerDebug("too short", result);
            return;
        }
        bool containsAllQueryUserPositions = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true)
            .Add(ctx.NaturalTitle).Add(desc).ContainsAllQueryUserPositions();
        if (!containsAllQueryUserPositions) {
            manager->ReplacerDebug("contains not all non stop user words", result);
            return;
        }
        manager->Commit(result, MRK_NEWS);
    }
}
