#pragma once

#include "word_accumulator.h"
#include "chain_accumulator.h"

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(PlaneAccumulator) {
        Y_CONST_FUNCTION ui32 CalcDist(ui32 leftPosition, ui32 rightPosition);
        Y_CONST_FUNCTION float CalcStdProximity(ui32 dist);
        Y_CONST_FUNCTION float CalcStdAttenuation(ui32 position);
        Y_CONST_FUNCTION float CalcProximity(size_t proximityId, ui32 dist);

        struct TQueryInfo {
            const TQuery& Query;
            const TWeights& WordWeights;
            size_t NumWords;
        };

        struct THitInfo {
            ui16 WordId;
            ui16 FormId;
            ui32 LeftPosition;
            ui32 RightPosition;
        };


        UNIT(TCoreUnit) {
            UNIT_STATE {
                const TQuery* Query = nullptr;
                TWeights WordWeights;

                size_t NumWords = Max<size_t>();

                bool IsExactMatch = false;
                float FormMatch = NAN;
                float IdfForm = NAN;

                ui32 LastPosition = Max<ui32>();
                ui32 FirstPosition = Max<ui32>();
                ui16 LastWordId = Max<ui16>();
                bool LastIsExactMatch = false;
                ui32 Dist = Max<ui32>();

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, NumWords);
                    SAVE_JSON_VAR(value, IsExactMatch);
                    SAVE_JSON_VAR(value, FormMatch);
                    SAVE_JSON_VAR(value, IdfForm);
                    SAVE_JSON_VAR(value, LastPosition);
                    SAVE_JSON_VAR(value, FirstPosition);
                    SAVE_JSON_VAR(value, LastWordId);
                    SAVE_JSON_VAR(value, LastIsExactMatch);
                    SAVE_JSON_VAR(value, Dist);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool&, const TQueryInfo& info) {
                    Query = &info.Query;
                    WordWeights = info.WordWeights;
                    NumWords = info.NumWords;
                }

                Y_FORCE_INLINE void NewDoc() {
                    LastPosition = Max<ui32>();
                    LastWordId = Max<ui16>();
                    LastIsExactMatch = false;
                    FirstPosition = Max<ui32>();
                }

                Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                    Y_ASSERT(Query);
                    Y_ASSERT(info.WordId < Query->Words.size());
                    Y_ASSERT(info.FormId < Query->Words[info.WordId].Forms.size());
                    FirstPosition = Min(FirstPosition, info.LeftPosition);

                    FormMatch = 0.0f;
                    IsExactMatch = false;

                    if (Query->Words[info.WordId].Forms[info.FormId]->MatchType == TMatch::OriginalWord) {
                        if (Query->Words[info.WordId].Forms[info.FormId]->MatchPrecision == TMatchPrecision::Exact) {
                            FormMatch = 1.0f;
                            IsExactMatch = true;
                        } else if (Query->Words[info.WordId].Forms[info.FormId]->MatchPrecision == TMatchPrecision::Lemma) {
                            FormMatch = 0.8f;
                        } else {
                            FormMatch = 0.1f;
                        }
                    } else {
                        FormMatch = 0.1f;
                    }
                    IdfForm = Query->Words[info.WordId].GetMainWeight(TRevFreq::Default) * FormMatch;
                    Dist = CalcDist(LastPosition, info.LeftPosition);
                }

                Y_FORCE_INLINE void FinishHit(const THitInfo& info) {
                    LastPosition = info.RightPosition;
                    LastWordId = info.WordId;
                    LastIsExactMatch = IsExactMatch;
                }

            };
        };

        UNIT(TPairMinDistUnit) {
            UNIT_STATE {
                TPoolPodHolder<ui32> PairMinDist;
                float PairMinProximity = NAN;
                float PairCohesion = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, PairMinDist);
                    SAVE_JSON_VAR(value, PairMinProximity);
                    SAVE_JSON_VAR(value, PairCohesion);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    PairMinDist.Init(pool, info.NumWords - 1, EStorageMode::Full);
                }

                Y_FORCE_INLINE void NewDoc() {
                    PairMinDist.Fill(Max<ui32>());
                }

                Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                    ui16 lastWordId = Vars<TCoreUnit>().LastWordId;
                    if ((lastWordId < Vars<TCoreUnit>().NumWords) && (info.WordId == (lastWordId + 1))) {
                        PairMinDist[lastWordId] = Min(PairMinDist[lastWordId], Vars<TCoreUnit>().Dist);
                    }
                }

                Y_FORCE_INLINE void FinishDoc() {
                    PairMinProximity = CalcPairMinProximity();
                    PairCohesion = CalcPairCohesion();
                }

                Y_FORCE_INLINE float CalcPairMinProximity() const {
                    if (Vars<TCoreUnit>().NumWords == 1) {
                        if (Vars<TCoreUnit>().FirstPosition < Max<ui32>()) {
                            return 1.0f;
                        }
                        return 0.0f;
                    }
                    Y_ASSERT(PairMinDist.Count() > 0);
                    float sum = 0.0f;
                    for (size_t i = 0; i != PairMinDist.Count(); ++i) {
                        sum += CalcStdProximity(PairMinDist[i]);
                    }
                    return sum / float(PairMinDist.Count());
                }

                Y_FORCE_INLINE float CalcPairCohesion() const {
                    if (Vars<TCoreUnit>().NumWords == 1) {
                        if (Vars<TCoreUnit>().FirstPosition < Max<ui32>()) {
                            return 1.0f;
                        }
                        return 0.0f;
                    }
                    const TQuery* query = Vars<TCoreUnit>().Query;
                    Y_ASSERT(query->Cohesion.size() == PairMinDist.Count());
                    float sum = 0.0f;
                    for (size_t i = 0; i != query->Cohesion.size(); ++i) {
                        if (PairMinDist[i] == 0) {
                            sum += 1.0f;
                        } else if (PairMinDist[i] < Max<ui32>()) {
                            const float cohesion = pow(1.0f - float(query->Cohesion[i]), float(PairMinDist[i]));
                            sum += cohesion;
                        }
                    }
                    return sum / float(query->Cohesion.size());
                }

            };
        };

        UNIT(TPairDistFirstAttenuationUnit) {
            UNIT_STATE {
                TPoolPodHolder<ui32> PairDist0FirstPosition;
                TPoolPodHolder<ui32> PairDist1FirstPosition;
                float PairDist0FirstAttenuation = NAN;
                float PairDist1FirstAttenuation = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, PairDist0FirstPosition);
                    SAVE_JSON_VAR(value, PairDist1FirstPosition);
                    SAVE_JSON_VAR(value, PairDist0FirstAttenuation);
                    SAVE_JSON_VAR(value, PairDist1FirstAttenuation);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    PairDist0FirstPosition.Init(pool, info.NumWords - 1, EStorageMode::Full);
                    PairDist1FirstPosition.Init(pool, info.NumWords - 1, EStorageMode::Full);
                }

                Y_FORCE_INLINE void NewDoc() {
                    PairDist0FirstPosition.Fill(Max<ui32>());
                    PairDist1FirstPosition.Fill(Max<ui32>());
                }

                Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                    ui16 lastWordId = Vars<TCoreUnit>().LastWordId;
                    if ((lastWordId < Vars<TCoreUnit>().NumWords) && (info.WordId == (lastWordId + 1))) {
                        if (Vars<TCoreUnit>().Dist <= 1) {
                            PairDist1FirstPosition[lastWordId] = Min(PairDist1FirstPosition[lastWordId], info.LeftPosition);
                            if (0 == Vars<TCoreUnit>().Dist) {
                                PairDist0FirstPosition[lastWordId] = Min(PairDist0FirstPosition[lastWordId], info.LeftPosition);
                            }
                        }
                    }
                }

                Y_FORCE_INLINE void FinishDoc() {
                    PairDist0FirstAttenuation = CalcPairDistFirstAttenuation(PairDist0FirstPosition);
                    PairDist1FirstAttenuation = CalcPairDistFirstAttenuation(PairDist1FirstPosition);
                }

                Y_FORCE_INLINE float CalcPairDistFirstAttenuation(const TPoolPodHolder<ui32>& pairDistFirstPosition) const {
                    if (Vars<TCoreUnit>().NumWords == 1) {
                        ui32 firstPosition = Vars<TCoreUnit>().FirstPosition;
                        if (firstPosition < Max<ui32>()) {
                            return CalcStdAttenuation(firstPosition);
                        }
                        return 0.0f;
                    }
                    Y_ASSERT(pairDistFirstPosition.Count() > 0);
                    float sum = 0.0f;
                    for (size_t i = 0; i != pairDistFirstPosition.Count(); ++i) {
                        if (pairDistFirstPosition[i] < Max<ui32>()) {
                            sum += CalcStdAttenuation(pairDistFirstPosition[i]);
                        }
                    }
                    return sum / float(pairDistFirstPosition.Count());
                }
            };
        };

        UNIT(TBclmUnit) {
            UNIT_STATE {
                TPoolPodHolder<TAccumulatorsByFloatValue> BclmFlatAcc;
                TAccumulatorsByFloatValue BclmWeightedAcc;
                TAccumulatorsByFloatValue BclmWeightedAttenAcc;
                float LastForm = NAN;
                float LastIdfForm = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, BclmFlatAcc);
                    SAVE_JSON_VAR(value, BclmWeightedAcc);
                    SAVE_JSON_VAR(value, BclmWeightedAttenAcc);
                    SAVE_JSON_VAR(value, LastForm);
                    SAVE_JSON_VAR(value, LastIdfForm);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    BclmFlatAcc.Init(pool, 3, EStorageMode::Full);
                    for (size_t i = 0; i != 3; ++i) {
                        BclmFlatAcc[i].Init(pool, info.NumWords);
                    }
                    BclmWeightedAcc.Init(pool, info.NumWords);
                    BclmWeightedAttenAcc.Init(pool, info.NumWords);
                }

                Y_FORCE_INLINE void NewDoc() {
                    for (size_t i = 0; i != 3; ++i) {
                        BclmFlatAcc[i].Assign(0.0f);
                    }
                    BclmWeightedAcc.Assign(0.0f);
                    BclmWeightedAttenAcc.Assign(0.0f);
                }

                Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                    ui16 lastWordId = Vars<TCoreUnit>().LastWordId;
                    float form = Vars<TCoreUnit>().FormMatch;
                    float idfForm = Vars<TCoreUnit>().IdfForm;
                    if ((lastWordId < Vars<TCoreUnit>().NumWords) && (info.WordId != lastWordId)) {
                        for (size_t i = 0; i != 3; ++i) {
                            const float proximity = CalcProximity(i, Vars<TCoreUnit>().Dist);
                            const float proximityRightForm = proximity * form;
                            const float proximityLeftForm = proximity * LastForm;
                            BclmFlatAcc[i][lastWordId] += proximityLeftForm;
                            BclmFlatAcc[i][info.WordId] += proximityRightForm;
                        }

                        const float stdProximity = CalcStdProximity(Vars<TCoreUnit>().Dist);
                        const float atten64 = 64.0f / (float(info.LeftPosition) + 64.0f);

                        BclmWeightedAcc[lastWordId] += LastForm * stdProximity * idfForm;
                        BclmWeightedAcc[info.WordId] += form * stdProximity * LastIdfForm;

                        BclmWeightedAttenAcc[lastWordId] += LastForm * stdProximity * idfForm * atten64;
                        BclmWeightedAttenAcc[info.WordId] += form * stdProximity * LastIdfForm * atten64;
                    }
                    LastForm = form;
                    LastIdfForm = idfForm;
                }
            };
        };

        template <size_t MaxChainLength, bool ExactMatchOnly, bool DirectMatchOnly>
        struct TChainAccGroup {
            UNIT_FAMILY(TChainFamily)

            UNIT(TChainUnit) {
                UNIT_FAMILY_STATE(TChainFamily) {
                    TChainAccumulator Chain;
                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, Chain);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                        Vars().Chain.NewQuery(pool, &info.Query, info.WordWeights);
                    }

                    Y_FORCE_INLINE void NewDoc() {
                        Vars().Chain.NewDoc();
                    }

                    Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                        if (ExactMatchOnly) {
                            if (!Vars<TCoreUnit>().IsExactMatch) {
                                return;
                            }
                        }

                        if (Vars<TCoreUnit>().LastWordId < Vars<TCoreUnit>().NumWords) {
                            if (Vars<TCoreUnit>().Dist > MaxChainLength) {
                                Vars().Chain.Start();
                            } else if (ExactMatchOnly) {
                                if (!Vars<TCoreUnit>().LastIsExactMatch) {
                                    Vars().Chain.Start();
                                }
                            } else if (DirectMatchOnly) {
                                if (info.WordId != (Vars<TCoreUnit>().LastWordId + 1)) {
                                    Vars().Chain.Start();
                                }
                            }
                        }
                        else {
                            Vars().Chain.Start();
                        }

                        Vars().Chain.Update(info.WordId, info.LeftPosition);
                    }
                };
            };

        };

#define UNIT_CHAIN(UnitName, MaxChainLength, ExactMatchOnly, DirectMatchOnly) \
    using T##UnitName##Family = TChainAccGroup<MaxChainLength, ExactMatchOnly, DirectMatchOnly>::TChainFamily; \
    using T##UnitName##Unit = TChainAccGroup<MaxChainLength, ExactMatchOnly, DirectMatchOnly>::TChainUnit;

        UNIT_CHAIN(Chain0,       0, false, false)
        UNIT_CHAIN(Chain0Exact,  0, true,  false)
        UNIT_CHAIN(Chain0Direct, 0, false, true)
        UNIT_CHAIN(Chain1, 1, false, false)
        UNIT_CHAIN(Chain2, 2, false, false)
        UNIT_CHAIN(Chain5, 5, false, false)

#undef UNIT_CHAIN

        UNIT(TAtten64Unit) {
            UNIT_STATE {
                //Positionless accumulators with attenuation
                // TODO: Move to positionless_accumulator_parts.h
                TAccumulatorsByFloatValue Atten64Acc;
                TAccumulatorsByFloatValue Atten64ExactAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, Atten64Acc);
                    SAVE_JSON_VAR(value, Atten64ExactAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    Atten64Acc.Init(pool, info.NumWords);
                    Atten64ExactAcc.Init(pool, info.NumWords);
                }
                Y_FORCE_INLINE void NewDoc() {
                    Atten64Acc.Assign(0.0f);
                    Atten64ExactAcc.Assign(0.0f);
                }
                Y_FORCE_INLINE void AddHit(const THitInfo& info) {
                    const float atten64 = 64.0f / (float(info.LeftPosition) + 64.0f);
                    const float atten64Form = atten64 * Vars<TCoreUnit>().FormMatch;
                    Atten64Acc[info.WordId] += atten64Form;
                    if (Vars<TCoreUnit>().IsExactMatch) {
                        Atten64ExactAcc[info.WordId] += atten64Form;
                    }
                }
            };
        };

        template <typename M>
        class TMotor : public M {
        public:
            using TModule = M;

        public:
            void NewQuery(TMemoryPool& pool, const TQuery& query, const TWeights& weights) {
                TQueryInfo info{query, weights, query.Words.size()};
                TModule::NewQuery(pool, info);
            }
            void AddHit(ui16 wordId, ui16 formId, ui32 leftPosition, ui32 rightPosition) {
                THitInfo info{wordId, formId, leftPosition, rightPosition};
                TModule::AddHit(info);
                TModule::FinishHit(info);
            }
            void NewDoc() {
                TModule::NewDoc();
            }
            void FinishDoc() {
                TModule::FinishDoc();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TPairMinDistUnit>& GetPairMinDistState() const {
                return M::template Vars<TPairMinDistUnit>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TPairDistFirstAttenuationUnit>& GetPairDistFirstAttenuationState() const {
                return M::template Vars<TPairDistFirstAttenuationUnit>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TBclmUnit>& GetBclmState() const {
                return M::template Vars<TBclmUnit>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TChain0Family>& GetChain0State() const {
                return M::template Vars<TChain0Family>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TChain0ExactFamily>& GetChain0ExactState() const {
                return M::template Vars<TChain0ExactFamily>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TChain0DirectFamily>& GetChain0DirectState() const {
                return M::template Vars<TChain0DirectFamily>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TChain1Family>& GetChain1State() const {
                return M::template Vars<TChain1Family>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TChain2Family>& GetChain2State() const {
                return M::template Vars<TChain2Family>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TChain5Family>& GetChain5State() const {
                return M::template Vars<TChain5Family>();
            }
            Y_FORCE_INLINE const typename M::template TGetState<TAtten64Unit>& GetAtten64State() const {
                return M::template Vars<TAtten64Unit>();
            }

        };
    }; //MACHINE_PARTS(PlaneAccumulator)
}; //NCore
}; //NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
