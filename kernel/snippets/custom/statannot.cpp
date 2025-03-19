#include "statannot.h"
#include "extended_length.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/trash_classifier/trash_classifier.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/delink/delink.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/simple_cmp/simple_cmp.h>
#include <kernel/snippets/static_annotation/static_annotation.h>

namespace NSnippets
{
    const TString TStaticAnnotationReplacer::TEXT_SRC = "static_annotation";

    void TStaticAnnotationReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& ctx = manager->GetContext();
        const TConfig& cfg = ctx.Cfg;
        if (cfg.GetStaticAnnotationMode() == SAM_DISABLED) {
            return;
        }

        if (ctx.MetaDescr.MayUse() && (ctx.Snip.Snips.empty() || ctx.IsByLink)) {
            return;
        }

        const bool useIfEmptySnip =
            (cfg.GetStaticAnnotationMode() & SAM_HIDE_EMPTY) &&
            ctx.Snip.Snips.empty();

        const bool useIfSnipIsByLink =
            (cfg.GetStaticAnnotationMode() & SAM_EXTENDED_BY_LINK) &&
            ctx.IsByLink;

        const bool killLink = CanKillByLinkSnip(ctx);

        if (useIfEmptySnip || useIfSnipIsByLink) {
            float staticAnnotationLength = 0;

            if (useIfSnipIsByLink && !ctx.Snip.Snips.empty() && !killLink) {
                staticAnnotationLength = ctx.LenCfg.GetNDesktopRowsLen(2);
            } else {
                staticAnnotationLength = ctx.LenCfg.GetMaxSnipLen();
            }

            TStaticAnnotation staticAnnotation(cfg, ctx.Markup);
            staticAnnotation.InitFromSentenceViewer(StatAnnotViewer, ctx.SuperNaturalTitle, ctx.QueryCtx);

            TUtf16String staticAnnot = staticAnnotation.GetSpecsnippet();
            TMultiCutResult extDesc;
            TSmartCutOptions options(ctx.Cfg);
            options.MaximizeLen = true;
            extDesc = CutDescription(staticAnnot, ctx, staticAnnotationLength, options);
            staticAnnot = extDesc.Short;

            bool isGoodAnnot = NTrashClassifier::IsGoodEnough(staticAnnot);
            if (staticAnnot.empty() || !isGoodAnnot) {
                if (!staticAnnot.empty()) {
                    manager->ReplacerDebug("not good enough", TReplaceResult().UseText(staticAnnot, TEXT_SRC));
                }
                return;
            }

            TReplaceResult res;
            if (useIfSnipIsByLink &&
                TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(staticAnnot).Add(ctx.SuperNaturalTitle).AddUrl(ctx.Url).Add(ctx.Snip) >
                TSimpleSnipCmp(ctx.QueryCtx, ctx.Cfg).Add(staticAnnot).Add(ctx.SuperNaturalTitle).AddUrl(ctx.Url))
            {
                if (!killLink) {
                    res.SetPreserveSnip();
                }
            }
            res.UseText(extDesc, TEXT_SRC);
            res.UseTitle(ctx.SuperNaturalTitle);
            manager->Commit(res, MRK_STATIC_ANNOTATION);
        }
    }
}
