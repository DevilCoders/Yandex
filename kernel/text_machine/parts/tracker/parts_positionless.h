#pragma once

#include "parts_base.h"

#include <kernel/text_machine/parts/accumulators/positionless_proxy_parts.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        UNIT_FAMILY_INFO_BEGIN(TPositionlessFamily, std::initializer_list<int> streams)
            UNIT_METHOD_INSTANTIATE_ARG(
                AddStreamHitBasic,
                ::NLingBoost::TStreamTypeDescr,
                StreamType,
                streams);
        UNIT_FAMILY_INFO_END()

        template <EHitWeightType HitWeight, EStreamType... Streams>
        struct TPositionlessGroup {
            UNIT_FAMILY(TPositionlessFamily)

            template <typename Proxy>
            UNIT(TPositionlessStub) {
                UNIT_FAMILY_STATE(TPositionlessFamily) {
                    Proxy PositionlessProxy;
                    size_t DocLength = Max<size_t>();

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, PositionlessProxy);
                        SAVE_JSON_VAR(value, DocLength);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    static void ScatterStatic(TScatterMethod::CollectStreams, const TStaticStreamsInfo& info) {
                        info.Streams.insert({Streams...});
                    }
                    static void ScatterStatic(TScatterMethod::CollectValueRefs, const TStaticValueRefsInfo& info) {
                        Proxy::ScatterStatic(TScatterMethod::CollectValueRefs(), TValueId{HitWeight, Streams...}, info);
                    }
                    void Scatter(TScatterMethod::BindValueRefs, const TBindValuesInfo& info) {
                        Vars().PositionlessProxy.Scatter(TScatterMethod::BindValueRefs(), TValueId{HitWeight, Streams...}, info);
                    }

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().PositionlessProxy.NewQuery(pool, info.Query);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().PositionlessProxy.NewDoc();
                        Vars().DocLength = 1;
                    }
                    template <EStreamType>
                    Y_FORCE_INLINE void AddStreamHitBasic(const THitInfo& hit) {
                        if (Vars<TCoreUnit>().FirstHitInStream) {
                            Vars().DocLength += hit.Position.Annotation->Stream->WordCount;
                        }
                        auto& form = *Vars<TCoreUnit>().Query->Words[hit.Word.WordId].Forms[hit.Word.FormId];
                        if (TMatch::Synonym == form.MatchType) {
                            Vars().PositionlessProxy.AddHitSynonym(hit.Word.WordId, hit.FormId, Vars<TCoreUnit>().HitWeights[HitWeight]);
                        }
                    }
                    Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                        Vars().PositionlessProxy.FinishDoc();
                    }

                public:
                    Y_FORCE_INLINE float CalcTRTxtBm25Synonym(float k) const {
                        return Vars().PositionlessProxy.AllAcc.CalcBm15(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), 1.0 + k * (Vars().DocLength / 350.0f));
                    }
                    Y_FORCE_INLINE float CalcTRTxtBm25(float k) const {
                        return Vars().PositionlessProxy.FormAcc.CalcBm15(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), 1.0f + k * (Vars().DocLength / 350.0f));
                    }
                    Y_FORCE_INLINE float CalcTRTxtBm25Exact(float k) const {
                        return Vars().PositionlessProxy.ExactAcc.CalcBm15(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), 1.0f + k * (Vars().DocLength / 350.0f));
                    }
                    Y_FORCE_INLINE float CalcTRTxtBm25SynonymW1(float k) const {
                        return Vars().PositionlessProxy.AllAcc.CalcBm15W0(
                            1.0 + k * (Vars().DocLength / 350.0f));
                    }
                    Y_FORCE_INLINE float CalcTRTxtBm25ExW1(float k) const {
                        return Vars().PositionlessProxy.ExactAcc.CalcBm15W0(
                            1.0 + k * (Vars().DocLength / 350.0f));
                    }
                    Y_FORCE_INLINE float CalcTRTextForms() const {
                        return Vars().PositionlessProxy.AllFormsMaskAcc.CalcTextAvgForms();
                    }
                    Y_FORCE_INLINE float CalcTRTextMaxForms() const {
                        return Vars().PositionlessProxy.AllFormsMaskAcc.CalcTextMaxForms();
                    }
                    Y_FORCE_INLINE float CalcTRTextWeightedForms() const {
                        return Vars().PositionlessProxy.AllFormsMaskAcc.CalcTextWeightedAvgForms(
                            Vars<TCoreUnit>().MainIdf);
                    }
                    Y_FORCE_INLINE float CalcTRNumWordsSynonym() const {
                        return Vars().PositionlessProxy.AllAcc.CountNonZeroFraction();
                    }
                    Y_FORCE_INLINE float CalcTRNumWordsExact() const {
                        return Vars().PositionlessProxy.ExactAcc.CountNonZeroFraction();
                    }

                    Y_FORCE_INLINE float CalcBm15(float k) const {
                        return Vars().PositionlessProxy.WeightedAcc.CalcBm15(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                    }
                    Y_FORCE_INLINE float CalcBm15ExactWX(float k) const {
                        return Vars().PositionlessProxy.ExactAcc.CalcBm15(
                            Vars<TCoreUnit>().ExactWeights.AsSeq4f(), k);
                    }
                    Y_FORCE_INLINE float CalcBm15AllW0(float k) const {
                        return Vars().PositionlessProxy.FormAcc.CalcBm15W0(k);
                    }
                    Y_FORCE_INLINE float CalcBm11(ui32 normalizer) const {
                        return Vars().PositionlessProxy.WeightedAcc.CalcBm11(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), Vars().DocLength, normalizer);
                    }
                    Y_FORCE_INLINE float CalcBm11ExactWX(ui32 normalizer) const {
                        return Vars().PositionlessProxy.ExactAcc.CalcBm11(
                            Vars<TCoreUnit>().ExactWeights.AsSeq4f(), Vars().DocLength, normalizer);
                    }
                    Y_FORCE_INLINE float CalcBm11AllW0(ui32 normalizer) const {
                        return Vars().PositionlessProxy.FormAcc.CalcBm11W0(Vars().DocLength, normalizer);
                    }
                    Y_FORCE_INLINE float CalcBm15StrictAnnotation(double k) const {
                        return Vars().PositionlessProxy.StrictWeightedAcc.CalcBm15(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                    }
                    Y_FORCE_INLINE float CalcBm15MaxAnnotation(double k) const {
                        return Vars().PositionlessProxy.WeightedAcc.CalcBm15Max(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                    }
                    Y_FORCE_INLINE float CalcWordCoverageExact() const {
                        return Vars().PositionlessProxy.ExactAcc.CalcCoverageEstimation(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), 1
                        );
                    }
                    Y_FORCE_INLINE float CalcWordCoverageForm() const {
                        return Vars().PositionlessProxy.FormAcc.CalcCoverageEstimation(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), 1
                        );
                    }
                    Y_FORCE_INLINE float CalcWordCoverageAny() const {
                        return Vars().PositionlessProxy.WeightedAcc.CalcCoverageEstimation(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(),
                            NPositionlessProxyParts::TWeightedUnit::OtherHitWeight
                        ) / NPositionlessProxyParts::TWeightedUnit::OtherHitWeight;
                    }
                };
            };
        };

        UNIT_FAMILY_INFO_BEGIN(TPositionlessAccFamily, std::initializer_list<int> streams)
            UNIT_METHOD_INSTANTIATE_ARG(
                AddStreamBlockHitBasic,
                ::NLingBoost::TStreamTypeDescr,
                StreamType,
                streams);
        UNIT_FAMILY_INFO_END()

        template <EHitWeightType HitWeight, EStreamType... Streams>
        struct TPositionlessAccGroup {
            UNIT_FAMILY(TPositionlessAccFamily)

            template <typename Accumulator>
            UNIT(TPositionlessAccStub) {
                UNIT_FAMILY_STATE(TPositionlessAccFamily) {
                    Accumulator PositionlessAccumulator;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, PositionlessAccumulator);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    static void ScatterStatic(TScatterMethod::CollectStreams, const TStaticStreamsInfo& info) {
                        info.Streams.insert({Streams...});
                    }
                    static void ScatterStatic(TScatterMethod::CollectValues, const TStaticValuesInfo& info) {
                        Accumulator::ScatterStatic(TScatterMethod::CollectValues(), TValueId{HitWeight, Streams...}, info);
                    }

                    void NewMultiQuery(TMemoryPool& pool, const TMultiQueryInfo& info) {
                         auto& queriesHelper = info.QueriesHelper;
                         Vars().PositionlessAccumulator.NewMultiQuery(pool, queriesHelper.GetNumBlocks());
                    }
                    void Scatter(TScatterMethod::RegisterValues, const TRegisterValuesInfo& info) {
                        Vars().PositionlessAccumulator.Scatter(TScatterMethod::RegisterValues(), TValueId{HitWeight, Streams...}, info);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().PositionlessAccumulator.NewDoc();
                    }
                    template <EStreamType>
                    Y_FORCE_INLINE void AddStreamBlockHitBasic(const TBlockHitInfo& hit) {
                        Vars().PositionlessAccumulator.AddHit(hit.BlockId,
                            hit.Precision, hit.FormId, Vars<TCoreUnit>().HitWeights[HitWeight]);
                    }
                    Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                        Vars().PositionlessAccumulator.FinishDoc();
                    }
                };
            };
        };
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
