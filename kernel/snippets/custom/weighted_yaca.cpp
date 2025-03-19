#include "weighted_yaca.h"
#include "replacer_weighter.h"
#include "yaca.h"
#include "extended_length.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/len_chooser.h>

#include <util/generic/string.h>

namespace NSnippets
{
    void TWeightedYacaCandidateProducer::Produce(ELanguage lang, TList<TSpecSnippetCandidate>& candidates)
    {
        if (!Context.Cfg.YacaSnippetReplacerAllowed()) {
            return;
        }
        if (IsYacaBanned(Context)) {
            return;
        }

        TYacaData yacaData(Context.DocInfos, lang, Context.Cfg);
        if (!yacaData.Title || !yacaData.Desc) {
            return;
        }

        candidates.push_back(TSpecSnippetCandidate());
        TSpecSnippetCandidate& candidate = candidates.back();
        candidate.Source = "yaca";
        TUtf16String desc = yacaData.Desc;
        TSmartCutOptions options(Context.Cfg);
        options.MaximizeLen = true;
        float maxLen = Context.LenCfg.GetMaxSpecSnipLen();
        float maxExtLen = GetExtSnipLen(Context.Cfg, Context.LenCfg);
        if (Context.Cfg.ExpFlagOn("no_extsnip_yaca")) {
            maxExtLen = 0;
        }
        candidate.SetExtendedText(SmartCutWithExt(desc, Context.IH, maxLen, maxExtLen, options));
        TUtf16String title = Context.Cfg.ExpFlagOn("switch_off_yaca_titles") ? Context.NaturalTitle.GetTitleString() : yacaData.Title;
        candidate.Title = MakeSpecialTitle(title, Context.Cfg, Context.QueryCtx);
        if (IsYacaForced(Context)) {
            candidate.Priority = 1;
        }
    }
}
