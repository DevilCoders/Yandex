#pragma once

#include "parts_base.h"

#include <kernel/text_machine/parts/accumulators/bow_accumulator_parts.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        UNIT_FAMILY_INFO_BEGIN(TBasicBagOfWordsFamily, std::initializer_list<int> streams)
            UNIT_METHOD_INSTANTIATE_ARG(
                AddStreamHitBasic,
                ::NLingBoost::TStreamTypeDescr,
                StreamType,
                streams);
        UNIT_FAMILY_INFO_END()

        template <EStreamType... Streams>
        struct TBasicBagOfWordsGroup {
            UNIT_FAMILY(TBasicBagOfWordsFamily)

            template <typename Accumulator>
            UNIT(TBasicBagOfWordsStub) {
                UNIT_FAMILY_STATE(TBasicBagOfWordsFamily) {
                    Accumulator BagOfWordsAccumulator;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, BagOfWordsAccumulator);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    static void ScatterStatic(TScatterMethod::CollectStreams, const TStaticStreamsInfo& info) {
                         info.Streams.insert({Streams...});
                    }
                    Y_FORCE_INLINE void NewMultiQuery(TMemoryPool& pool,
                        const TMultiQueryInfo& multiQueryInfo)
                    {
                        Y_ASSERT(multiQueryInfo.BagOfWords);

                        Vars().BagOfWordsAccumulator.NewMultiQuery(pool,
                            multiQueryInfo.CoreState,
                            multiQueryInfo.QueriesHelper.GetMultiQuery(),
                            *multiQueryInfo.BagOfWords);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().BagOfWordsAccumulator.NewDoc();
                    }
                    template <EStreamType Stream>
                    Y_FORCE_INLINE void AddStreamHitBasic(const THitInfo& hit) {
                        Vars().BagOfWordsAccumulator.AddHit(hit);
                    }
                    Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                        Vars().BagOfWordsAccumulator.FinishDoc();
                    }

                public:
                    Y_FORCE_INLINE float CalcCosineMaxMatch() const {
                        return Vars().BagOfWordsAccumulator.GetCosineMatchState().MaxMatchW1S2;
                    }
                    Y_FORCE_INLINE float CalcAnnotationMatchAvgValue() const {
                        return Vars().BagOfWordsAccumulator.GetAnnotationMatchState().AnnotationMatchAccumulator.CalcMatch95AvgValue();
                    }
                    Y_FORCE_INLINE float CalcFullMatchWMax() const {
                        return Vars().BagOfWordsAccumulator.GetFullMatchState().FullMatchAcc.CalcMatch95MaxValue();
                    }
                    Y_FORCE_INLINE float CalcCosineMatchMaxPrediction() const {
                        return Vars().BagOfWordsAccumulator.GetCosineMatchState().CosineSimilarityAccW2S2.CalcMaxPrediction();
                    }
                    Y_FORCE_INLINE float CalcCosineMatchWeightedValue() const {
                        return Vars().BagOfWordsAccumulator.GetCosineMatchState().CosineSimilarityAccW2S2.CalcWeightedValue();
                    }
                    Y_FORCE_INLINE float CalcOriginalRequestFractionExact() const {
                        return Vars().BagOfWordsAccumulator.GetBagFractionExactState().OriginalRequestBagFraction;
                    }
                    Y_FORCE_INLINE float CalcOriginalRequestFraction() const {
                        return Vars().BagOfWordsAccumulator.GetBagFractionState().OriginalRequestBagFraction;
                    }
                };
            };
        };
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
