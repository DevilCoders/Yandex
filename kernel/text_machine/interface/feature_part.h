#pragma once

#include "query.h"
#include "hit.h"

#include <kernel/text_machine/structured_id/typed_param.h>

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/typetraits.h>
#include <util/generic/hash.h>
#include <util/digest/multi.h>

namespace NTextMachine {
    namespace NFeatureInternals {
        struct TKValueTraits {
            using TValue = float;

            static Y_FORCE_INLINE TStringBuf GetName() {
                return TStringBuf("K");
            }
        };

        struct TNormValueTraits {
            using TValue = ui32;

            static Y_FORCE_INLINE TStringBuf GetName() {
                return TStringBuf("Norm");
            }
        };

        struct TStreamIndexValueTraits {
            using TValue = ui16;

            static Y_FORCE_INLINE TStringBuf GetName() {
                return TStringBuf("StreamIdx");
            }
        };
    } // NFeatureInternals

    /*
     * Names of elements, that could be used in text machine descriptor ("*.in" file)
     */
    struct TFeaturePart {
        enum EFeaturePartType {
            RawId,
            NormValue,
            KValue,
            Algorithm,
            Stream,
            StreamIndex,
            StreamValue,
            Expansion,
            Filter,
            Accumulator,
            Normalizer,
            TrackerPrefix,
            RegionClass,
            FeaturePartMax
        };
    };
    using EFeaturePartType = TFeaturePart::EFeaturePartType;

    struct TTrackerPrefix {
        enum ETrackerPrefixType {
            BagOfWords
        };
    };
    using ETrackerPrefixType = TTrackerPrefix::ETrackerPrefixType;

    struct TAlgorithm {
        enum EAlgorithmType {
            NumX,
            AvgW,
            TotalW,
            MaxW,
            MinW,

            Bm11,
            Bm11ExactWX,
            Bm11AllW0,
            Bm15,
            Bm15ExactWX,
            Bm15AllW0,
            Bm15FLog,
            Bm15FLogExactWX,
            Bm15FLogW0,
            Bm15MaxAnnotation,               // weighted Bm15Max with weight of the word by (match weighted * annotation value)
            Bm15StrictAnnotation,            // weighted Bm15 with weight of the word by (strict match weight * annotation value)
            TRBclmLite,
            TRTxtPairSynonym,
            TRTxtPair,
            TRTxtPairExact,
            TRTxtPairW1,
            TRNumWordsSynonym,
            TRNumWordsExact,
            TRTextWeightedForms,
            TRTextForms,
            TRTextMaxForms,
            TRTxtBreakSynonym,
            TRTxtBreak,
            TRTxtBreakExact,
            TRTxtBm25,
            TRTxtBm25Exact,
            TRTxtBm25Synonym,
            TRTxtBm25SynonymW1,
            TRTxtBm25ExactW1,
            Atten64Bm15,
            Atten64Bm15ExactWX,

            /*
             * Bocm algorithms are based on applying penalty calculated from
             * - word position discrepancy  in query and in sentence calculated as PositionAdjust = 1 / (1 + delta^2)
             * - word form discrepancy (exact, lemma, etc.)
             */
            Bocm,
            Bocm11,                          // BM11( [sum_over_sentence(word_idfs)*PosFormAdjust*AnnValue], [weights], normalizer )
            Bocm15,                          // BM15( [sum_over_sentence(word_idfs)*PosFormAdjust*AnnValue], [weights], k )
            BocmDouble,                      // BM15( [sum_over_sentence(PosFormAdjust)*PosFormAdjust*AnnValue], [weights], k ) - like Bocm15, but instead of SumIdfVals uses sum(PosFormAdjust)

            FullMatchValue,
            FullMatchWMax,
            FullMatchAnyValue,               // max value of annotation that have the same words (or forms/one word synonyms) as query and in the same order
            FullMatchOriginalWordValue,      // max value of annotation that have the same words (or forms) as query and in the same order

            AllWcmWeightedValue,             // average of ([annotation value] * [sum of weights of matched words in the annotation])
            AllWcmMatch80AvgValue,           // same as AllWcmAvg, but average is taken only by annotations with [sum of weights of matched words in the annotation] > 0.8
            AllWcmMatch95AvgValue,           // same as AllWcmAvg, but average is taken only by annotations with [sum of weights of matched words in the annotation] > 0.95
            AllWcmMaxPrediction,
            AllWcmWeightedPrediction,        // fraction [sum by match^2 * annotation value] / [sum by match], there match = [sum of weights of matched words in the annotation] aka ValueAvg
            AllWcmMaxMatch,                  // max(Match) = max([sum of weights of matched words in the annotation])
            AllWcmMaxMatchValue,             // annotation value of the sentence with max match, in other words MatchValue for max(Match) aka ValueMax

            MixMatchWeightedValue,
            AnnotationMatchWeightedValue,
            TRBreakAny,
            TRBreakExact,
            AnnotationMatchAvgValue,
            AnnotationMaxValue,              // max value of annotation that have been fully covered by the query
            AnnotationMaxValueWeighted,      // max product of (value of fully covered annotation) and (the measure of closeness between word orders in the annotation and the query)
            AnnotationMatchMaxWeightedValue, // fraction [sum by match^2 * annotation value] / [sum by match], there match = [the measure of closeness between word orders in the annotation and the query]
            CosineMaxMatch,
            CosineMatchMaxPrediction,
            CosineMatchWeightedValue,
            ExactQueryMatchAvgValue,
            AnyQueryMatchAvgValue,
            ExactQueryMatchCount,
            AnyQueryMatchCount,

            BclmPlaneProximity1,
            BclmPlaneProximity0WX,
            BclmPlaneProximity1Bm15W0Size1,    // Bm15W0 over Bclm accumulators (if size of query is one, words matching accumulator is used)
            BclmWeightedProximity1Bm15Size1,   // Bm15 over Bclm accumulators (if size of query is one, weighted words matching accumulator is used)
            BclmWeighted,
            BclmWeightedAtten,
            BclmWeightedFLogW0,
            BclmMixPlain,                     // Bm15Mix of BclmFlat accumulator and unweighted word-form accumulator

            Chain0Wcm,
            Chain0ExactWcm,
            Chain0DirectWcm,
            Chain1Wcm,
            Chain2Wcm,
            Chain5Wcm,

            PairMinProximity,
            PairDist1FirstAttenuation,
            PairCohesion,
            PairDist0FirstAttenuation,
            QueryPrefixMatchOriginalWordValue, // max value of annotation that have the same words (or forms) in prefix as words in query, and checks them to be in the same order
            MinWindowSize,
            FullMatchCoverageMinOffset,
            FullMatchCoverageStartingPercent,
            OriginalRequestFractionExact,
            OriginalRequestFraction,
            PerWordAMMaxValueMin,

            CMMatchTop5AvgPrediction,
            CMMatchTop5AvgMatch,
            CMMatchTop5AvgValue,
            CMMatchTop5AvgMatchValue,
            CMMatch80AvgValue,

            PerWordCMMaxPredictionMin,
            PerWordCMMaxMatchMin,

            WordCoverageAny,
            WordCoverageForm,
            WordCoverageExact,

            MinPerTrigramMaxValueAny,
            AvgPerTrigramMaxValueAny,
            AvgPerTrigramAvgValueAny,
            QueryPartMatchSumValueAny,
            QueryPartMatchMaxValueAny,

            AlgorithmMax,
        };
    };
    using EAlgorithmType = TAlgorithm::EAlgorithmType;

    struct TStreamValue {
        enum EStreamValueType {
            V0,      // x --> 1.0 (x ^ 0)
            V2,      // x --> x ^ 2
            V4,      // x --> x ^ 4
            AttenV1, // x --> x / (1 + pos in sentence)
            OldTRAtten, // 100 / (breakId + 100)
            TxtHead, //brk == 1
            TxtHiRel, //rel >= 2
            VMax
        };
    };
    using EStreamValueType = TStreamValue::EStreamValueType;

    struct TFilter {
        enum EFilterType {
            All,
            Top,
            WTop,
            Tail,
            WTail,
            Bulk,
            WBulk,
            FilterMax
        };
    };
    using EFilterType = TFilter::EFilterType;

    struct TAccumulator {
        enum EAccumulatorType {
            Count,
            SumW,
            MaxW,
            MinW,
            SumF,
            SumWF,
            SumW2F,
            MaxF,
            MaxWF,
            MinF,
            MinWF,
            AccumulatorMax
        };
    };
    using EAccumulatorType = TAccumulator::EAccumulatorType;

    // NOTE. It is important that for each
    // normalizer type there is corresponding
    // accumulator type with the same name.
    // This property is used in aggregator units design.
    struct TNormalizer {
        enum ENormalizerType {
            None, // For compatibility with obsolete core
            Count,
            SumW,
            SumF,
            MaxW,
            MaxF,
            NormalizerMax
        };
    };
    using ENormalizerType = TNormalizer::ENormalizerType;

    using TKValue = ::NStructuredId::TParam<NFeatureInternals::TKValueTraits>;
    using TNormValue = ::NStructuredId::TParam<NFeatureInternals::TNormValueTraits>;
    using TStreamIndexValue = ::NStructuredId::TParam<NFeatureInternals::TStreamIndexValueTraits>;

    class TStreamSet {
    public:
        using EStreamType = NLingBoost::EStreamType;

        enum EStreamSetType {
            FieldSet1,
            FieldSet2,
            FieldSet3,
            FieldSet4,
            FieldSetUT,
            FieldSetBagOfWords,
            Text,
            LinksAll,
            FieldSet5,
            StreamSetMax
        };

    public:
        TStreamSet() = default;
        TStreamSet(const TStreamSet& rhs) = default;
        TStreamSet& operator = (const TStreamSet& rhs) = default;

        Y_FORCE_INLINE TStreamSet(EStreamType value)
            : Stream(value)
        {}
        Y_FORCE_INLINE TStreamSet(EStreamSetType value)
            : StreamSet(value)
        {}

        Y_FORCE_INLINE bool operator == (const TStreamSet& rhs) const {
            return Stream == rhs.Stream && StreamSet == rhs.StreamSet;
        }
        Y_FORCE_INLINE bool operator < (const TStreamSet& rhs) const {
            return (IsStreamSet() && !rhs.IsStreamSet())
                || (IsStreamSet() && rhs.IsStreamSet() && StreamSet < rhs.StreamSet)
                || (IsStream() && rhs.IsStream() && Stream < rhs.Stream);
        }
        Y_FORCE_INLINE bool IsStream() const {
            return Stream != NLingBoost::TStream::StreamMax;
        }
        Y_FORCE_INLINE bool IsStreamSet() const {
            return StreamSet != TStreamSet::StreamSetMax;
        }
        Y_FORCE_INLINE EStreamType GetStream() const {
            return Stream;
        }
        Y_FORCE_INLINE EStreamSetType GetStreamSet() const {
            return StreamSet;
        }

    private:
        EStreamType Stream = NLingBoost::TStream::StreamMax;
        EStreamSetType StreamSet = StreamSetMax;
    };
    using EStreamSetType = TStreamSet::EStreamSetType;
} // NTextMachine

template <>
struct THash<NTextMachine::TStreamSet> {
    ui64 operator() (const NTextMachine::TStreamSet& x) {
        return MultiHash(x.GetStreamSet(), x.GetStream());
    }
};
