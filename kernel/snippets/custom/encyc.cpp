#include "encyc.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/delink/delink.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <kernel/tarc/docdescr/docdescr.h>

#include <util/generic/string.h>

namespace NSnippets
{
    static const TString HEADLINE_SRC = "encyc";

    class TEncycData {
    public:
        TUtf16String Desc;
        TUtf16String Title;

        TEncycData(const TDocInfos& infos) {
            TStaticData data(infos, "encyc");
            Desc = data.Attrs["desc"];
            Title = data.Attrs["title"];
        }
    };

    void TEncycReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        TEncycData encycData(ctx.DocInfos);
        if (!encycData.Desc) {
            return;
        }
        bool replaced = false;
        TReplaceResult result;
        TSnipTitle newTitle;

        bool want = false;
        bool wantTitle = false;
        if (encycData.Title) {
            newTitle = MakeSpecialTitle(encycData.Title, ctx.Cfg, ctx.QueryCtx);
            if (TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.NaturalTitle) <=
                TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(encycData.Title))
            {
                wantTitle = true;
                result.UseTitle(newTitle);
                replaced = true;
            } else {
                manager->ReplacerDebug("title contains less query words than original one", TReplaceResult().UseText(TUtf16String(), HEADLINE_SRC).UseTitle(newTitle));
            }
        }

        const TSnipTitle& usedTitle = wantTitle ? newTitle : ctx.NaturalTitle;
        const bool killLink = CanKillByLinkSnip(ctx);
        double oldWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(ctx.NaturalTitle).Add(killLink ? TSnip() : ctx.Snip).AddUrl(ctx.Url).GetWeight();
        double newWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(usedTitle).Add(encycData.Desc).AddUrl(ctx.Url).GetWeight();
        if (oldWeight <= newWeight * 2.0) {
            want = true;
        } else {
            manager->ReplacerDebug("contains less query words than the half of original", TReplaceResult().UseText(encycData.Desc, HEADLINE_SRC));
        }

        if (want || ctx.IsByLink) {
            TUtf16String desc = encycData.Desc;
            SmartCut(desc, ctx.IH, ctx.LenCfg.GetMaxSpecSnipLen(), TSmartCutOptions(ctx.Cfg));
            if (!want && !killLink) {
                result.SetPreserveSnip();
            }
            result.UseText(desc, HEADLINE_SRC);
            replaced = true;
        }

        if (replaced) {
            manager->Commit(result, MRK_ENCYC);
        }
    }
}
