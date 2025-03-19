#include "replacer_chooser.h"

#include "replacer_weighter.h"
#include "weighted_yaca.h"
#include "weighted_dmoz.h"
#include "weighted_meta.h"
#include "weighted_icatalog.h"
#include "weighted_videodescr.h"

#include <kernel/snippets/config/config.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {

namespace {

template<class TProducer>
void DoReplace(TReplaceManager* manager, const EMarker marker, ECompareAlgo algo = COMPARE_MXNET) {
    const TReplaceContext& ctx = manager->GetContext();
    if (ctx.Cfg.GetUILang() != LANG_RUS) {
        return;
    }

    TVector<ELanguage> langs;
    langs.push_back(LANG_RUS);
    if (ctx.DocLangId != LANG_UNK && ctx.DocLangId != LANG_RUS) {
        langs.push_back(ctx.DocLangId);
    }

    TProducer producer(ctx);
    for (ELanguage lang : langs) {
        TList<TSpecSnippetCandidate> candidates;
        producer.Produce(lang, candidates);
        TReplacerWeighter weighter(ctx, lang);
        ESubReplacerResult verdict = weighter.PerformReplace(*manager, marker, candidates, algo);
        if (verdict != ReplacementNotYetFound) {
            break;
        }
    }
}

} // anonymous namespace

ECompareAlgo AlgoByConfig(const TString& paramPrefix, TReplaceManager* manager)
{
    const auto& cfg = manager->GetContext().Cfg;
    ECompareAlgo algo = COMPARE_MXNET;
    if (cfg.ExpFlagOn(paramPrefix + "_weighter_algo_hilite")) {
        algo = COMPARE_HILITE;
    } else if (cfg.ExpFlagOn(paramPrefix + "_weighter_algo_random")) {
        algo = COMPARE_RANDOM;
    }
    return algo;
}

void TWeightedYaCatalogReplacer::DoWork(TReplaceManager* manager) {
    DoReplace<TWeightedYacaCandidateProducer>(manager, MRK_WEIGHTED_YACA);
}

void TWeightedIdealCatalogReplacer::DoWork(TReplaceManager* manager) {
    DoReplace<TWeightedIdealCatalogCandidateProducer>(manager, MRK_WEIGHTED_YACA, AlgoByConfig("icatalog", manager));
}

void TWeightedMetaDescrReplacer::DoWork(TReplaceManager* manager) {
    DoReplace<TWeightedMetaCandidateProducer>(manager, MRK_WEIGHTED_METADESCR_AND_DMOZ);
}

void TWeightedVideoDescrReplacer::DoWork(TReplaceManager* manager) {
    DoReplace<TWeightedVideoDescrCandidateProducer>(manager, MRK_VIDEO_DESCR, AlgoByConfig("videodescr", manager));
}

} // namespace NSnippets
