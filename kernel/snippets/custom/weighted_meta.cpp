#include "weighted_meta.h"
#include "replacer_weighter.h"
#include "extended_length.h"

#include <kernel/snippets/custom/data/yaca_list.h>
#include <kernel/snippets/cut/cut.h>
#include <kernel/snippets/replace/replace.h>
#include <kernel/snippets/sent_match/lang_check.h>
#include <kernel/snippets/sent_match/len_chooser.h>
#include <kernel/snippets/sent_match/meta.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <util/generic/string.h>

namespace NSnippets
{

void TWeightedMetaCandidateProducer::Produce(ELanguage lang, TList<TSpecSnippetCandidate>& candidates)
{
    const TUtf16String srcText = Context.MetaDescr.GetTextCopy();
    if (!srcText) {
        return;
    }
    if (Context.Snip.ContainsMetaDescr()) {
        return;
    }
    if (lang != LANG_UNK && !HasWordsOfAlphabet(srcText, lang)) {
        return;
    }

    candidates.push_back(TSpecSnippetCandidate());
    TSpecSnippetCandidate& candidate = candidates.back();

    TSmartCutOptions options(Context.Cfg);
    options.MaximizeLen = true;
    float maxLen = Context.LenCfg.GetMaxSnipLen();
    float maxExtLen = GetExtSnipLen(Context.Cfg, Context.LenCfg);
    if (Context.Cfg.ExpFlagOn("no_extsnip_meta")) {
        maxExtLen = 0;
    }
    candidate.SetExtendedText(SmartCutWithExt(srcText, Context.IH, maxLen, maxExtLen, options));
    candidate.Source = "meta_descr";

    bool forced = TForcedYacaUrls::GetDefault().Contains(Context.Url);
    if (forced) {
        candidate.Priority = 1;
    }
}

};
