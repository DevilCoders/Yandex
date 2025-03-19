#pragma once

#include "parts_base.h"

#include <kernel/text_machine/parts/accumulators/annotation_accumulator_motor.h>
#include <kernel/text_machine/parts/accumulators/annotation_accumulator_parts_for_neuro_text_counters.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        UNIT_FAMILY_INFO_BEGIN(TAnnotationFamily, std::initializer_list<int> streams)
            UNIT_METHOD_INSTANTIATE_ARG(
                AddStreamHitBasic,
                ::NLingBoost::TStreamTypeDescr,
                StreamType,
                streams);
        UNIT_FAMILY_INFO_END()

        template <EStreamType... Streams>
        struct TAnnotationGroup {
            UNIT_FAMILY(TAnnotationFamily)

            template <typename Accumulator>
            UNIT(TAnnotationStub) {
                UNIT_FAMILY_STATE(TAnnotationFamily) {
                    Accumulator AnnotationAccumulator;
                    size_t DocLength = Max<size_t>();
                    TQueryWordId NumWords = Max<TQueryWordId>();

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, AnnotationAccumulator);
                        SAVE_JSON_VAR(value, DocLength);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    static void ScatterStatic(TScatterMethod::CollectStreams, const TStaticStreamsInfo& info) {
                        info.Streams.insert({Streams...});
                    }
                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().AnnotationAccumulator.NewQuery(pool, &info.Query,
                            &Vars<TCoreUnit>().MainWeights);
                        Vars().NumWords = info.Query.GetNumWords();
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().AnnotationAccumulator.NewDoc();
                        Vars().DocLength = 1;
                    }
                    template <EStreamType Stream>
                    Y_FORCE_INLINE void AddStreamHitBasic(const THitInfo& hit) {
                        if (Vars<TCoreUnit>().FirstHitInStream) {
                            Vars().DocLength += hit.Position.Annotation->Stream->WordCount;
                        }
                        Vars().AnnotationAccumulator.AddHit(hit);
                    }
                    Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                        Vars().AnnotationAccumulator.FinishDoc();
                    }

                public:
                    Y_FORCE_INLINE float CalcBocm11(ui32 normalizer) const {
                        return Vars().AnnotationAccumulator.GetAnyBocmState().Values.CalcBm11(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(),
                            Vars().DocLength, normalizer);
                    }
                    Y_FORCE_INLINE float CalcBocm15(float k) const {
                        return Vars().AnnotationAccumulator.GetAnyBocmState().Values.CalcBm15(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                    }
                    Y_FORCE_INLINE float CalcBocmDouble(float k) const {
                        return Vars().AnnotationAccumulator.GetAnyBocmState().DoubleValues.CalcBm15(Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                    }
                    Y_FORCE_INLINE float CalcFullMatchValue() const {
                        return Vars().AnnotationAccumulator.GetExactFullMatchState().GetFullValue();
                    }
                    Y_FORCE_INLINE float CalcFullMatchAnyValue() const {
                        return Vars().AnnotationAccumulator.GetAnyFullMatchState().GetFullValue();
                    }
                    Y_FORCE_INLINE float CalcFullMatchOriginalWordValue() const {
                        return Vars().AnnotationAccumulator.GetOriginalFullMatchState().GetFullValue();
                    }
                    Y_FORCE_INLINE float CalcAllWcmMatch95AvgValue() const {
                        return Vars().AnnotationAccumulator.GetAnyWcmState().WcmAcc.CalcMatch95AvgValue();
                    }
                    Y_FORCE_INLINE float CalcAllWcmMatch80AvgValue() const {
                        return Vars().AnnotationAccumulator.GetAnyWcmState().WcmAcc.CalcMatch80AvgValue();
                    }
                    Y_FORCE_INLINE float CalcAllWcmMaxPrediction() const {
                        return Vars().AnnotationAccumulator.GetAnyWcmState().WcmAcc.CalcMaxPrediction();
                    }
                    Y_FORCE_INLINE float CalcAllWcmWeightedPrediction() const {
                        return Vars().AnnotationAccumulator.GetAnyWcmState().WcmAcc.CalcWeightedPrediction();
                    }
                    Y_FORCE_INLINE float CalcAllWcmMaxMatch() const {
                        return Vars().AnnotationAccumulator.GetAnyWcmState().WcmAcc.CalcMaxMatch();
                    }
                    Y_FORCE_INLINE float CalcAllWcmMaxMatchValue() const {
                        return Vars().AnnotationAccumulator.GetAnyWcmState().WcmAcc.CalcMaxMatchValue();
                    }
                    Y_FORCE_INLINE float CalcAllWcmWeightedValue() const {
                        return Vars().AnnotationAccumulator.GetAnyWcmState().WcmAcc.CalcWeightedValue();
                    }
                    Y_FORCE_INLINE float CalcMixMatchWeightedValue() const {
                        return Vars().AnnotationAccumulator.GetAnyAnnotationMatchState().MixMatchAcc.CalcWeightedValue();
                    }
                    Y_FORCE_INLINE float CalcAnnotationMatchWeightedValue() const {
                        return Vars().AnnotationAccumulator.GetAnyAnnotationMatchState().AnnotationMatchAcc.CalcWeightedValue();
                    }
                    Y_FORCE_INLINE float CalcCosineMatchMaxPrediction() const {
                        return Vars().AnnotationAccumulator.GetAnyAnnotationMatchState().CosineMatchAcc.CalcMaxPrediction();
                    }
                    Y_FORCE_INLINE float CalcAnnotationMaxValue() const {
                        return Vars().AnnotationAccumulator.GetAnyAnnotationMatchState().AnnotationMatchAcc.CalcMaxAnyValue();
                    }
                    Y_FORCE_INLINE float CalcAnnotationMaxValueWeighted() const {
                        return Vars().AnnotationAccumulator.GetAnyAnnotationMatchState().AnnotationMatchAcc.CalcMaxPrediction();
                    }
                    Y_FORCE_INLINE float CalcExactQueryMatchAvgValue() const {
                        return Vars().AnnotationAccumulator.GetExactInfixMatchState().MatchAcc.CalcMatch95AvgValue();
                    }
                    Y_FORCE_INLINE float CalcAnyQueryMatchAvgValue() const {
                        return Vars().AnnotationAccumulator.GetAnyInfixMatchState().MatchAcc.CalcMatch95AvgValue();
                    }
                    Y_FORCE_INLINE float CalcExactQueryMatchCount() const {
                        return Vars().AnnotationAccumulator.GetExactInfixMatchState().MatchAcc.CalcMatch95Count();
                    }
                    Y_FORCE_INLINE float CalcAnyQueryMatchCount() const {
                        return Vars().AnnotationAccumulator.GetAnyInfixMatchState().MatchAcc.CalcMatch95Count();
                    }
                    Y_FORCE_INLINE float CalcQueryPrefixMatchOriginalWordValue() const {
                        return Vars().AnnotationAccumulator.GetOriginalFullMatchState().GetPrefixValue();
                    }
                    Y_FORCE_INLINE float CalcPerWordAMMaxValueMin() const {
                        return Vars().AnnotationAccumulator.GetAnyPerWordAnnotationMatchState().MinMaxValue;
                    }
                    Y_FORCE_INLINE float CalcCMMatchTop5AvgPrediction() const {
                        return Vars().AnnotationAccumulator.GetAnyCMMatchState().MatchAcc.CalcTop5AvgPrediction();
                    }
                    Y_FORCE_INLINE float CalcCMMatchTop5AvgMatch() const {
                        return Vars().AnnotationAccumulator.GetAnyCMMatchState().MatchAcc.CalcTop5AvgMatch();
                    }
                    Y_FORCE_INLINE float CalcCMMatchTop5AvgValue() const {
                        return Vars().AnnotationAccumulator.GetAnyCMMatchState().MatchAcc.CalcTop5AvgValue();
                    }
                    Y_FORCE_INLINE float CalcCMMatchTop5AvgMatchValue() const {
                        return Vars().AnnotationAccumulator.GetAnyCMMatchState().MatchAcc.CalcTop5AvgMatch() *
                               Vars().AnnotationAccumulator.GetAnyCMMatchState().MatchAcc.CalcTop5AvgValue();
                    }
                    Y_FORCE_INLINE float CalcCMMatch80AvgValue() const {
                        return Vars().AnnotationAccumulator.GetAnyCMMatchState().MatchAcc.CalcMatch80AvgValue();
                    }
                    Y_FORCE_INLINE float CalcPerWordCMMaxPredictionMin() const {
                        return Vars().AnnotationAccumulator.GetAnyCMMatchState().MinMaxPredictValue;
                    }
                    Y_FORCE_INLINE float CalcPerWordCMMaxMatchMin() const {
                        return Vars().AnnotationAccumulator.GetAnyCMMatchState().MinMaxMatchValue;
                    }
                    Y_FORCE_INLINE float CalcMinPerTrigramMaxValueAny() const {
                        return Vars().AnnotationAccumulator.GetAnyTrigramState().MinPerNgramMaxValue;
                    }
                    Y_FORCE_INLINE float CalcAvgPerTrigramMaxValueAny() const {
                        return Vars().AnnotationAccumulator.GetAnyTrigramState().AvgPerNgramMaxValue;
                    }
                    Y_FORCE_INLINE float CalcAvgPerTrigramAvgValueAny() const {
                        return Vars().AnnotationAccumulator.GetAnyTrigramState().AvgPerNgramAvgValue;
                    }
                    Y_FORCE_INLINE float CalcQueryPartMatchSumValueAny() const {
                        return Vars().AnnotationAccumulator.GetAnyQueryPartMatchState().MatchAcc.CalcMatchAllSumValue();
                    }
                    Y_FORCE_INLINE float CalcQueryPartMatchMaxValueAny() const {
                        return Vars().AnnotationAccumulator.GetAnyQueryPartMatchState().MatchAcc.CalcMaxAnyValue();
                    }

                    //Factors from old text relevance
                    Y_FORCE_INLINE float CalcTRBclmLite() const {
                        return Vars().AnnotationAccumulator.GetAnyTRBclmLiteState().BclmAcc.CalcBm15W0(0.06);
                    }

                    Y_FORCE_INLINE float CalcTRTxtPairSynonym() const {
                        return Vars().AnnotationAccumulator.GetAnyTRTxtPairState().PairAcc.CalcBm15(Vars<TCoreUnit>().PairWordWeights.AsSeq4f(), 1.0);
                    }
                    Y_FORCE_INLINE float CalcTRTxtPair() const {
                        return Vars().AnnotationAccumulator.GetOriginalTRTxtPairState().PairAcc.CalcBm15(Vars<TCoreUnit>().PairWordWeights.AsSeq4f(), 1.0);
                    }
                    Y_FORCE_INLINE float CalcTRTxtPairExact() const {
                        return Vars().AnnotationAccumulator.GetExactTRTxtPairState().PairAcc.CalcBm15(Vars<TCoreUnit>().PairWordWeights.AsSeq4f(), 1.0);
                    }
                    Y_FORCE_INLINE float CalcTRTxtPairW1() const {
                        return Vars().AnnotationAccumulator.GetAnyTRTxtPairState().PairAcc.CalcBm15W0(1.0);
                    }

                    Y_FORCE_INLINE float CalcTRTxtBreakSynonym() const {
                        float res = Vars().AnnotationAccumulator.GetAnyAnnotationMatchState().QueryMatchCount;
                        return res / (res + 1.0);
                    }
                    Y_FORCE_INLINE float CalcTRTxtBreak() const {
                        float res = Vars().AnnotationAccumulator.GetOriginalAnnotationMatchState().QueryMatchCount;
                        return res / (res + 1.0);
                    }
                    Y_FORCE_INLINE float CalcTRTxtBreakExact() const {
                        float res = Vars().AnnotationAccumulator.GetExactAnnotationMatchState().QueryMatchCount;
                        return res / (res + 1.0);
                    }


                };
            };
        };
    } // MACHINE_PARTS(Tracker)

} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
