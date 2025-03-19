#include "weighted_dmoz.h"
#include "dmoz.h"
#include "replacer_weighter.h"
#include "extended_length.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <util/generic/string.h>

namespace NSnippets {
    static constexpr size_t DMOZ_MIN_LENGTH = 100;

    TWeightedDmozCandidateProducer::TWeightedDmozCandidateProducer(const TReplaceContext& context)
        : Context(context)
    {
        if (!Context.Cfg.DmozSnippetReplacerAllowed()) {
            return;
        }
        if (DmozBanned(Context.Cfg, Context.DocInfos)) {
            return;
        }
        DmozData.Reset(new TDmozData(Context.DocInfos));
    }

    TWeightedDmozCandidateProducer::~TWeightedDmozCandidateProducer() {
    }

    void TWeightedDmozCandidateProducer::Produce(ELanguage lang, TList<TSpecSnippetCandidate>& candidates) {
        TUtf16String title;
        TUtf16String desc;
        if (!DmozData || !DmozData->FindByLanguage(lang, title, desc)) {
            return;
        }
        if (Context.Cfg.ExpFlagOff("enable_more_dmoz") && lang == ELanguage::LANG_RUS &&
            !Context.IsByLink && desc.size() < DMOZ_MIN_LENGTH && desc.size() < Context.Snip.GetRawTextWithEllipsis().size())
        {
            return;
        }

        candidates.push_back(TSpecSnippetCandidate());
        TSpecSnippetCandidate& candidate = candidates.back();

        candidate.Title = MakeSpecialTitle(title, Context.Cfg, Context.QueryCtx);
        TSmartCutOptions options(Context.Cfg);
        options.MaximizeLen = true;
        float maxLen = Context.LenCfg.GetMaxSpecSnipLen();
        float maxExtLen = GetExtSnipLen(Context.Cfg, Context.LenCfg);
        if (Context.Cfg.ExpFlagOn("no_extsnip_dmoz")) {
            maxExtLen = 0;
        }
        candidate.SetExtendedText(SmartCutWithExt(desc, Context.IH, maxLen, maxExtLen, options));
        candidate.Source = "dmoz";
    }
}
