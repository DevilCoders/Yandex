#include "market.h"

#include <kernel/snippets/archive/staticattr.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/read_helper/read_helper.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/length.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/similarity.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/smartcut/cutparam.h>
#include <kernel/snippets/titles/make_title/util_title.h>

namespace NSnippets {
    static const TString HEADLINE_SRC = "market_text";

    static bool ReplaceTitle(TReplaceManager* manager, const TUtf16String& rawTitle, TSnipTitle& newTitle) {
        const TReplaceContext& ctx = manager->GetContext();
        newTitle = MakeSpecialTitle(rawTitle, ctx.Cfg, ctx.QueryCtx);
        if (GetSimilarity(ctx.NaturalTitle.GetEQInfo(), newTitle.GetEQInfo(), true) >= 2.0 / 3.0) {
            return false; // natural title is relevant to described offer, don't replace
        }
        TReadabilityHelper readHelper(ctx.Cfg.UseTurkishReadabilityThresholds());
        readHelper.AddSnip(newTitle.GetTitleSnip());
        if (readHelper.CalcReadability() >= 0.5) {
            double natWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true).
                Add(ctx.SuperNaturalTitle).GetWeight();
            double newWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true).
                Add(newTitle).GetWeight();
            if (newWeight < natWeight) {
                return false;
            }
        }
        return true;
    }

    static TUtf16String ReplaceSnip(TReplaceManager* manager, TUtf16String rawDesc, const TSnipTitle& snipTitle) {
        const TReplaceContext& ctx = manager->GetContext();
        TWordSpanLen wordSpanLen(TCutParams::Symbol());
        float snipLength = wordSpanLen.CalcLength(ctx.Snip.Snips);
        size_t minLength = Min(static_cast<size_t>(80), static_cast<size_t>(snipLength));

        if (!rawDesc || rawDesc.size() < minLength) {
            manager->ReplacerDebug("too short", TReplaceResult().UseText(rawDesc, HEADLINE_SRC));
            return TUtf16String();
        }

        rawDesc.to_upper(0, 1);
        bool isAlgoSnip = true;
        TUtf16String newSnip = CutSnip(rawDesc, ctx.Cfg, ctx.QueryCtx, ctx.LenCfg.GetMaxSpecSnipLen(), ctx.LenCfg.GetMaxSnipLen(), nullptr, ctx.SnipWordSpanLen);
        if (newSnip.size() >= minLength) {
            TSmartCutOptions options(ctx.Cfg);
            options.MaximizeLen = true;
            SmartCut(newSnip, ctx.IH, ctx.LenCfg.GetMaxSpecSnipLen(), options);
        } else {
            isAlgoSnip = false;
            newSnip = rawDesc;
            SmartCut(newSnip, ctx.IH, ctx.LenCfg.GetMaxSpecSnipLen(), TSmartCutOptions(ctx.Cfg));
        }

        TReadabilityChecker checker(ctx.Cfg, ctx.QueryCtx);
        checker.CheckBadCharacters = true;
        checker.CheckCapitalization = true;
        if (!checker.IsReadable(newSnip, isAlgoSnip)) {
            manager->ReplacerDebug("doesn't pass readability check", TReplaceResult().UseText(newSnip, HEADLINE_SRC));
            return TUtf16String();
        }

        double oldWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true).
            Add(snipTitle).Add(ctx.Snip).GetWeight();
        double newWeight = TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg, true, true).
            Add(snipTitle).Add(newSnip).GetWeight();
        if (newWeight < oldWeight) {
            manager->ReplacerDebug("contains less non stop query words than original one", TReplaceResult().UseText(newSnip, HEADLINE_SRC));
            return TUtf16String();
        }

        return newSnip;
    }

    void TMarketReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();

        TStaticData data(ctx.DocInfos, "market");
        TUtf16String title = data.Attrs["title"];
        TUtf16String desc = data.Attrs["description"];
        if (!title && !desc) {
            return;
        }

        TSnipTitle newTitle;
        bool replaceTitle = ReplaceTitle(manager, title, newTitle);
        const TSnipTitle& snipTitle = replaceTitle ? newTitle : ctx.NaturalTitle;
        TUtf16String newSnip = ReplaceSnip(manager, desc, snipTitle);

        if (newSnip || replaceTitle) {
            TReplaceResult res;
            if (replaceTitle) {
                res.UseTitle(newTitle);
            }
            if (newSnip) {
                res.UseText(newSnip, HEADLINE_SRC);
            } else if (ctx.MetaDescr.MayUse() && (ctx.Snip.Snips.empty() || ctx.IsByLink)) {
                TUtf16String meta = ctx.MetaDescr.GetTextCopy();
                TSmartCutOptions options(ctx.Cfg);
                options.MaximizeLen = true;
                SmartCut(meta, ctx.IH, ctx.LenCfg.GetMaxSnipLen(), options);
                if (ctx.IsByLink) {
                    res.SetPreserveSnip();
                }
                res.UseText(meta, "meta_descr");
            }
            manager->Commit(res, MRK_MARKET);
        }
    }
}
