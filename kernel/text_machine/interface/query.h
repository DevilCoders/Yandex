#pragma once

#include "types.h"

#include <kernel/lingboost/constants.h>
#include <kernel/lingboost/freq.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/system/defaults.h>

#include <array>

namespace NTextMachineProtocol{
    class TInExpansionWordWeigthsRewrite;
}

namespace NTextMachine {
    using NLingBoost::TExpansion;
    using NLingBoost::EExpansionType;

    using NLingBoost::TRegionClass;
    using NLingBoost::ERegionClassType;

    using NLingBoost::TRegionId;

    using NLingBoost::TMatchPrecision;
    using NLingBoost::EMatchPrecisionType;

    using NLingBoost::TMatch;
    using NLingBoost::EMatchType;

    using NLingBoost::FormClassToPrecision;

    using NLingBoost::TIdfsByType;
    using NLingBoost::TWordFreq;
    using NLingBoost::TRevFreq;
    using NLingBoost::EWordFreqType;

    struct TQueryWordMatch {
        TIdfsByType IdfsByType; // Match IDF
        EMatchType MatchType;
        EMatchPrecisionType MatchPrecision;
        size_t MatchedBlockId; // Integer that uniquely identifies basic block (word or phrase) that produces this match
        size_t SynonymTypeMask;
        float Weight; // Prob("match is correct"), e.g. how likely that lemma L is correct for query word W
    };

    struct TQueryWord {
        TIdfsByType IdfsByType; // Aggregate word IDF (as is from rich tree)
        TVector<TQueryWordMatch*> Forms;
        TString Text;

        TMaybe<float> RewriteMainWeight;
        TMaybe<float> RewriteExactWeight;

        float GetMainWeight(EWordFreqType freqType) const {
            return RewriteMainWeight.GetOrElse(IdfsByType[freqType]);
        }

        float GetExactWeight(EWordFreqType freqType) const {
            if (RewriteExactWeight) {
                return *RewriteExactWeight;
            }

            for (auto& form : Forms) {
                if ((TMatch::OriginalWord == form->MatchType) && (TMatchPrecision::Exact == form->MatchPrecision)) {
                    return form->IdfsByType[freqType];
                }
            }
            return 0.f;
        }
    };

    struct TQueryType {
        ERegionClassType RegionClass;
    };

    struct TQueryValue {
        TQueryType Type;
        float Value;
    };

    struct TQuery {
        EExpansionType ExpansionType;
        TVector<TQueryValue> Values;

        TVector<TQueryWord> Words;
        TVector<float> Cohesion;
        ELanguage MainLanguage;
        size_t Index;
        size_t IndexInBundle;

        TString ReconstructedQuery;

        bool IsOriginal() const {
            return TExpansion::OriginalRequest == ExpansionType;
        }
        float GetMaxValue() const {
            float maxValue = 0.0f;
            for (const auto& value : Values) {
                maxValue = Max(value.Value, maxValue);
            }
            return maxValue;
        }
        TQueryWordId GetNumWords() const {
            Y_ASSERT(Words.size() <= Max<TQueryWordId>());
            return Words.size();
        }
        TWordFormId GetNumForms(TQueryWordId wordId) const {
            Y_ASSERT(Words[wordId].Forms.size() <= Max<TWordFormId>());
            return Words[wordId].Forms.size();
        }
        EMatchType GetMatch(TQueryWordId wordId, TWordFormId formId) const {
            return Words[wordId].Forms[formId]->MatchType;
        }
        EMatchPrecisionType GetMatchPrecision(TQueryWordId wordId, TWordFormId formId) const {
            return Words[wordId].Forms[formId]->MatchPrecision;
        }
        float GetIdf(TQueryWordId wordId, EWordFreqType freqType = TWordFreq::Default) const {
            return Words[wordId].IdfsByType[freqType];
        }

        TStringBuf GetReconstructedQuery() {
            if (ReconstructedQuery) {
                return ReconstructedQuery;
            } else {
                DoReconstructQuery();
                return ReconstructedQuery;
            }
        }
    private:
        void DoReconstructQuery();
    };

    struct TMultiQuery {
        TVector<TQuery> Queries;
        void ApplyWeightsRewrites(TArrayRef<const NTextMachineProtocol::TInExpansionWordWeigthsRewrite> rewrites);
    };

    void MakeDummyQuery(TQuery& multiQuery);
    void MakeDummyQuery(TMultiQuery& multiQuery);
} // NTextMachine
