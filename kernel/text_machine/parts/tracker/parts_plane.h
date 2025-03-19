#pragma once

#include "parts_base.h"
#include "positionless_units.h"

#include <kernel/text_machine/parts/accumulators/plane_accumulator_parts.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        UNIT_FAMILY_INFO_BEGIN(TPlaneAccumulatorFamily)
            UNIT_METHOD_INSTANTIATE_ARG(
                AddStreamHitBasic,
                ::NLingBoost::TStreamTypeDescr,
                StreamType,
                TStream::Body);
        UNIT_FAMILY_INFO_END()

        struct TPlaneAccumulatorGroup {
            UNIT_FAMILY(TPlaneAccumulatorFamily)

            template <typename Accumulator>
            UNIT(TPlaneAccumulatorStub) {
                UNIT_FAMILY_STATE(TPlaneAccumulatorFamily) {
                    Accumulator PlaneAccumulator;
                    size_t BodyLength = Max<size_t>();

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, PlaneAccumulator);
                        SAVE_JSON_VAR(value, BodyLength);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    static void ScatterStatic(TScatterMethod::CollectStreams, const TStaticStreamsInfo& info) {
                        info.Streams.insert(TStream::Body);
                    }

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().PlaneAccumulator.NewQuery(pool, info.Query,
                            Vars<TCoreUnit>().MainWeights);
                    }
                    Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                        Vars().PlaneAccumulator.NewDoc();
                        Vars().BodyLength = 1;
                    }
                    template<EStreamType StreamType>
                    Y_FORCE_INLINE void AddStreamHitBasic(const THitInfo& hit) {
                        static_assert(StreamType == TStream::Body, "");
                        Vars().PlaneAccumulator.AddHit(hit.Word.WordId, hit.Word.FormId,
                            hit.Position.Annotation->FirstWordPos + hit.Position.LeftWordPos,
                            hit.Position.Annotation->FirstWordPos + hit.Position.RightWordPos);
                    }

                    Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                        Vars().PlaneAccumulator.FinishDoc();
                        if (Vars<TCoreUnit>().Streams[TStream::Body]) {
                            Vars().BodyLength = Vars<TCoreUnit>().Streams[TStream::Body]->WordCount + 1;
                        }
                    }

                public:
                    Y_FORCE_INLINE float CalcBclmPlaneProximity1(ui32 normalizer) const {
                        if (Vars<TCoreUnit>().Query->Words.size() > 1) {
                            return Vars().PlaneAccumulator.GetBclmState().BclmFlatAcc[1].CalcBm11(
                                Vars<TCoreUnit>().MainWeights.AsSeq4f(),
                                Vars().BodyLength, normalizer);
                        } else {
                            return Vars<TBodyPositionlessFamily>().PositionlessProxy.WeightedAcc.CalcBm11(
                                Vars<TCoreUnit>().MainWeights.AsSeq4f(),
                                Vars().BodyLength, normalizer);
                        }
                    }
                    Y_FORCE_INLINE float CalcBclmPlaneProximity0WX(ui32 normalizer) const {
                        if (Vars<TCoreUnit>().Query->Words.size() > 1) {
                            return Vars().PlaneAccumulator.GetBclmState().BclmFlatAcc[0].CalcBm11(
                                Vars<TCoreUnit>().ExactWeights.AsSeq4f(),
                                Vars().BodyLength, normalizer);
                        } else {
                            return Vars<TBodyPositionlessFamily>().PositionlessProxy.WeightedAcc.CalcBm11(
                                Vars<TCoreUnit>().ExactWeights.AsSeq4f(),
                                Vars().BodyLength, normalizer);
                        }
                    }
                    Y_FORCE_INLINE float CalcBclmWeighted(ui32 normalizer) const {
                        if (Vars<TCoreUnit>().Query->Words.size() > 1) {
                            return Vars().PlaneAccumulator.GetBclmState().BclmWeightedAcc.CalcBm11(
                                Vars<TCoreUnit>().MainWeights.AsSeq4f(),
                                Vars().BodyLength, normalizer);
                        } else {
                            return Vars<TBodyPositionlessFamily>().PositionlessProxy.WeightedAcc.CalcBm11(
                                Vars<TCoreUnit>().MainWeights.AsSeq4f(),
                                Vars().BodyLength, normalizer);
                        }
                    }
                    Y_FORCE_INLINE float CalcBclmWeightedAtten(float k) const {
                        return Vars().PlaneAccumulator.GetBclmState().BclmWeightedAttenAcc.CalcBm15(
                            Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                    }
                    Y_FORCE_INLINE float CalcChain0Wcm() const {
                        return Vars().PlaneAccumulator.GetChain0State().Chain.CalcMaxWcm();
                    }
                    Y_FORCE_INLINE float CalcChain0ExactWcm() const {
                        return Vars().PlaneAccumulator.GetChain0ExactState().Chain.CalcMaxWcm();
                    }
                    Y_FORCE_INLINE float CalcChain1Wcm() const {
                        return Vars().PlaneAccumulator.GetChain1State().Chain.CalcMaxWcm();
                    }
                    Y_FORCE_INLINE float CalcChain0DirectWcm() const {
                        return Vars().PlaneAccumulator.GetChain0DirectState().Chain.CalcMaxWcm();
                    }
                    Y_FORCE_INLINE float CalcChain2Wcm() const {
                        return Vars().PlaneAccumulator.GetChain2State().Chain.CalcMaxWcm();
                    }
                    Y_FORCE_INLINE float CalcChain5Wcm() const {
                        return Vars().PlaneAccumulator.GetChain5State().Chain.CalcMaxWcm();
                    }
                    Y_FORCE_INLINE float CalcPairMinProximity() const {
                        return Vars().PlaneAccumulator.GetPairMinDistState().PairMinProximity;
                    }
                    Y_FORCE_INLINE float CalcPairDist1FirstAttenuation() const {
                        return Vars().PlaneAccumulator.GetPairDistFirstAttenuationState().PairDist1FirstAttenuation;
                    }
                    Y_FORCE_INLINE float CalcPairCohesion() const {
                        return Vars().PlaneAccumulator.GetPairMinDistState().PairCohesion;
                    }
                    Y_FORCE_INLINE float CalcPairDist0FirstAttenuation() const {
                        return Vars().PlaneAccumulator.GetPairDistFirstAttenuationState().PairDist0FirstAttenuation;
                    }
                 };
            };
        };

        using TPlaneAccumulatorFamily = TPlaneAccumulatorGroup::TPlaneAccumulatorFamily;
        template <typename Accumulator>
        using TPlaneAccumulatorStub = TPlaneAccumulatorGroup::TPlaneAccumulatorStub<Accumulator>;
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>

