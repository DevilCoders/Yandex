#include "trash_annotation.h"
#include "trash_viewer.h"
#include "statannot.h"

#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/custom/trash_classifier/trash_classifier.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/tsnip.h>
#include <kernel/snippets/title_trigram/title_trigram.h>

#include <util/charset/wide.h>

namespace NSnippets
{
    const TString TTrashAnnotationReplacer::TEXT_SRC = "trash_annotation";
    static const int TRASH_ANNOTATION_VIEW_SYMBOLS = 250;

    static TUtf16String GetSnipFromData(const TReplaceContext& repCtx, const TTrashViewer& trashViewer,
            const TSnipTitle& title) {
        const TArchiveView& view = trashViewer.GetResult();
        TArchiveView fview = GetFirstSentences(view, TRASH_ANNOTATION_VIEW_SYMBOLS);
        if (fview.Empty()) {
            return TUtf16String();
        }

        TRetainedSentsMatchInfo data;
        data.SetView(&repCtx.Markup, fview, TRetainedSentsMatchInfo::TParams(repCtx.Cfg, repCtx.QueryCtx).SetPutDot().SetParaTables());
        const TSentsInfo& sentsInfo = *data.GetSentsInfo();
        const TSentsMatchInfo& sentsMatchInfo = *data.GetSentsMatchInfo();
        TTitleMatchInfo titleMatchInfo;
        titleMatchInfo.Fill(sentsMatchInfo, &title);

        int i = 0;
        for (; i < sentsInfo.SentencesCount(); ++i) {
            int w0 = sentsInfo.FirstWordIdInSent(i);
            int w1 = sentsInfo.LastWordIdInSent(i);
            if (titleMatchInfo.GetTitleLikeness(w0, w1) < 0.2) {
                break;
            }
        }
        if (i >= sentsInfo.SentencesCount()) {
            return TUtf16String();
        }
        return sentsInfo.GetTextWithEllipsis(sentsInfo.FirstWordIdInSent(i), sentsInfo.WordCount() - 1);
    }

    void TTrashAnnotationReplacer::DoWork(TReplaceManager* manager) {
        const TReplaceContext& repCtx = manager->GetContext();
        if (repCtx.MetaDescr.MayUse() || (!repCtx.Snip.Snips.empty() && !repCtx.IsByLink)) {
                // Trash statannot is worse then meta description
            return;
        }

        TUtf16String trashAnnot = GetSnipFromData(repCtx, TrashViewer, repCtx.SuperNaturalTitle);
        TSmartCutOptions options(repCtx.Cfg);
        options.MaximizeLen = true;
        SmartCut(trashAnnot, repCtx.IH, repCtx.LenCfg.GetNDesktopRowsLen(1), options);

        bool isGoodAnnot = NTrashClassifier::IsGoodEnough(trashAnnot);
        if (trashAnnot.empty() || !isGoodAnnot) {
            if (!trashAnnot.empty()) {
                manager->ReplacerDebug("not good enough", TReplaceResult().UseText(trashAnnot, TEXT_SRC));
            }
            return;
        }

        TReplaceResult res;
        if (repCtx.IsByLink) {
            res.SetPreserveSnip();
        }
        res.UseText(trashAnnot, TEXT_SRC);
        res.UseTitle(repCtx.SuperNaturalTitle);
        manager->Commit(res, MRK_TRASH_ANNOTATION);
    }
}
