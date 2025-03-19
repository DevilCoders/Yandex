#include "weighted_videodescr.h"
#include "replacer_weighter.h"
#include "extended_length.h"
#include "yaca.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/len_chooser.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/string.h>

namespace NSnippets
{
    void TWeightedVideoDescrCandidateProducer::Produce(ELanguage lang, TList<TSpecSnippetCandidate>& candidates)
    {
        Y_UNUSED(lang);

        if (!Context.Cfg.ExpFlagOn("weighted_videodescr")) {
            return;
        }

        const auto attr = Context.DocInfos.find("videodescr");
        if (attr == Context.DocInfos.end()) {
            return;
        }

        // TODO: Check for BROKEN_RUNEs
        TUtf16String snip = UTF8ToWide<true>(attr->second);
        if (!snip) {
            return;
        }

        candidates.push_back(TSpecSnippetCandidate());
        TSpecSnippetCandidate& candidate = candidates.back();
        candidate.Source = "videodescr";
        TSmartCutOptions options(Context.Cfg);
        options.MaximizeLen = true;
        float maxLen = Context.LenCfg.GetMaxSpecSnipLen();
        float maxExtLen = GetExtSnipLen(Context.Cfg, Context.LenCfg);
        if (Context.Cfg.ExpFlagOff("weighted_videodescr_ext")) {
            maxExtLen = 0;
        }

        candidate.SetExtendedText(SmartCutWithExt(snip, Context.IH, maxLen, maxExtLen, options));
    }
}
