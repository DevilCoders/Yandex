#pragma once

#include "positionless_accumulator_parts.h"

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(PositionlessProxy) {
        using NPositionlessAccumulatorParts::TValueItems;
        using NPositionlessAccumulatorParts::TValueFormItems;

        UNIT(TRefUnit) {
            UNIT_STATE {
                const TQuery* Query;
                TAccumulatorsByFloatRef ExactAcc;
                TAccumulatorsByFloatRef LemmaAcc;
                TAccumulatorsByFloatRef OtherAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, ExactAcc);
                    SAVE_JSON_VAR(value, LemmaAcc);
                    SAVE_JSON_VAR(value, OtherAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                static void ScatterStatic(TScatterMethod::CollectValueRefs, const TValueId& id, const TStaticValueRefsInfo& info) {
                    TValueItems<TValueRefEntry> items(info.ValueRefs);

                    items.Exact.Id = TValueId(id, THitPrec::Exact);
                    items.Lemma.Id = TValueId(id, THitPrec::Lemma);
                    items.Other.Id = TValueId(id, THitPrec::Other);

                    MACHINE_LOG("PositionlessProxy::TRefUnit", "CollectFloatRefs", TStringBuilder{}
                        << "{Id: " << id.FullName() << "}");
                }
                void Scatter(TScatterMethod::BindValueRefs, const TValueId& id, const TBindValuesInfo& info) {
                    using namespace NPositionlessAccumulatorParts;

                    TValueItems<const TValueRefEntry> items(info.ValueRefs);

                    Y_ASSERT(items.Exact.Id == TValueId(id, THitPrec::Exact));
                    Y_ASSERT(items.Lemma.Id == TValueId(id, THitPrec::Lemma));
                    Y_ASSERT(items.Other.Id == TValueId(id, THitPrec::Other));

                    ExactAcc.BindToZero();
                    LemmaAcc.BindToZero();
                    OtherAcc.BindToZero();

                    MACHINE_LOG("PositionlessProxy::TRefUnit", "BindValue", TStringBuilder{}
                        << "{Id: " << id.FullName()
                        << ", ExactIndex: " << items.Exact.ValueIndex
                        << ", LemmaIndex: " << items.Lemma.ValueIndex
                        << ", OtherIndex: " << items.Other.ValueIndex);

                    for (size_t i: xrange(Query->Words.size())) {
                        auto& word = Query->Words[i];

                        for (auto& formPtr : word.Forms) {
                            Y_ASSERT(formPtr);
                            auto blockId = formPtr->MatchedBlockId;
                            if (TMatch::OriginalWord == formPtr->MatchType) {
                                EHitPrecType hitType = GetHitPrec(formPtr->MatchPrecision);

                                switch (hitType) {
                                    case THitPrec::Exact: {
                                        ExactAcc.Bind(i, info.FloatsRegistry.GetChecked(items.Exact.ValueIndex, blockId));
                                        break;
                                    }
                                    case THitPrec::Lemma: {
                                        LemmaAcc.Bind(i, info.FloatsRegistry.GetChecked(items.Lemma.ValueIndex, blockId));
                                        break;
                                    }
                                    case THitPrec::Other: {
                                        OtherAcc.Bind(i, info.FloatsRegistry.GetChecked(items.Other.ValueIndex, blockId));
                                        break;
                                    }
                                    default: {
                                        Y_ASSERT(false);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQuery& query) {
                    Query = &query;
                    const size_t numWords = Query->Words.size();
                    ExactAcc.Init(pool, numWords);
                    LemmaAcc.Init(pool, numWords);
                    OtherAcc.Init(pool, numWords);
                }
            };
        };

        UNIT(TSynonymUnit) {
            UNIT_STATE {
                TAccumulatorsByFloatValue SynonymAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, SynonymAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQuery& query) {
                    const size_t numWords = query.Words.size();
                    SynonymAcc.Init(pool, numWords);
                }
                Y_FORCE_INLINE void NewDoc() {
                    SynonymAcc.Assign(0.0f);
                }
                Y_FORCE_INLINE void AddHitSynonym(ui16 wordId, ui16 /*formId*/, float value) {
                    SynonymAcc[wordId] += value;
                }
            };
        };

        UNIT(TSynonymFormUnit) {
            UNIT_STATE {
                TAccumulatorsByUintValue SynFormsMaskAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, SynFormsMaskAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQuery& query) {
                    const size_t numWords = query.Words.size();
                    SynFormsMaskAcc.Init(pool, numWords);
                }
                Y_FORCE_INLINE void NewDoc() {
                    SynFormsMaskAcc.Assign(0);
                }
                Y_FORCE_INLINE void AddHitSynonym(const ui16 wordId, const ui16 formId, float /*value*/) {
                    SynFormsMaskAcc[wordId] |= ((ui64)1 << (formId & 63));
                }
            };
        };

        UNIT(TRefFormUnit) {
            UNIT_STATE {
                const TQuery* Query;
                TAccumulatorsByUintRef FormsMaskAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, FormsMaskAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                static void ScatterStatic(TScatterMethod::CollectValueRefs, const TValueId& id, const TStaticValueRefsInfo& info) {
                    TValueFormItems<TValueRefEntry> items(info.ValueRefs);

                    items.FormsMask.Id = TValueId(id, TFormsCount::All);

                    MACHINE_LOG("PositionlessProxy::TRefUnit", "CollectFloatRefs", TStringBuilder{}
                        << "{Id: " << id.FullName() << "}");
                }
                void Scatter(TScatterMethod::BindValueRefs, const TValueId& id, const TBindValuesInfo& info) {
                    using namespace NPositionlessAccumulatorParts;

                    TValueFormItems<const TValueRefEntry> items(info.ValueRefs);

                    Y_ASSERT(items.FormsMask.Id == TValueId(id, TFormsCount::All));

                    FormsMaskAcc.BindToZero();

                    MACHINE_LOG("PositionlessProxy::TRefUnit", "BindValue", TStringBuilder{}
                        << "{Id: " << id.FullName()
                        << ", FormsMaskIndex: " << items.FormsMask.ValueIndex << "}");

                    for (size_t i: xrange(Query->Words.size())) {
                        auto& word = Query->Words[i];

                        for (auto& formPtr : word.Forms) {
                            Y_ASSERT(formPtr);
                            auto blockId = formPtr->MatchedBlockId;
                            if (TMatch::OriginalWord == formPtr->MatchType) {
                                FormsMaskAcc.Bind(i, info.Uints64Registry.GetChecked(items.FormsMask.ValueIndex, blockId));
                            }
                        }
                    }
                }

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQuery& query) {
                    Query = &query;
                    const size_t numWords = Query->Words.size();
                    FormsMaskAcc.Init(pool, numWords);
                }
            };
        };

        UNIT(TFormUnit) {
            UNIT_STATE {
                TAccumulatorsByFloatValue FormAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, FormAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQuery& query) {
                    const size_t numWords = query.Words.size();
                    FormAcc.Init(pool, numWords);
                }
                Y_FORCE_INLINE void FinishDoc() {
                    FormAcc.CopyFrom(
                        NSeq4f::Add(
                            Vars<TRefUnit>().ExactAcc.AsSeq4f(),
                            Vars<TRefUnit>().LemmaAcc.AsSeq4f()
                        )
                    );
                }
            };
        };

        UNIT(TWeightedUnit) {
            constexpr static float ExactHitWeight = 1.f;
            constexpr static float LemmaHitWeight = 0.8f;
            constexpr static float OtherHitWeight = 0.1f;

            UNIT_STATE {
                TAccumulatorsByFloatValue WeightedAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, WeightedAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQuery& query) {
                    const size_t numWords = query.Words.size();
                    WeightedAcc.Init(pool, numWords);
                }
                Y_FORCE_INLINE void FinishDoc() {
                    WeightedAcc.CopyFrom(
                        NSeq4f::Add(
                            NSeq4f::Add(
                                Vars<TRefUnit>().ExactAcc.AsSeq4f(),
                                Vars<TRefUnit>().LemmaAcc.AsSeq4f(), LemmaHitWeight
                            ),
                            NSeq4f::Add(
                                Vars<TRefUnit>().OtherAcc.AsSeq4f(),
                                Vars<TSynonymUnit>().SynonymAcc.AsSeq4f()
                            ),
                            OtherHitWeight
                        )
                    );
                }
            };
        };

        UNIT(TStrictWeightedUnit) {
            UNIT_STATE {
                TAccumulatorsByFloatValue StrictWeightedAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, StrictWeightedAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQuery& query) {
                    const size_t numWords = query.Words.size();
                    StrictWeightedAcc.Init(pool, numWords);
                }
                Y_FORCE_INLINE void FinishDoc() {
                    StrictWeightedAcc.CopyFrom(
                        NSeq4f::Add(
                            NSeq4f::Add(
                                Vars<TRefUnit>().ExactAcc.AsSeq4f(),
                                Vars<TRefUnit>().LemmaAcc.AsSeq4f(), 0.1f
                            ),
                            NSeq4f::Add(
                                Vars<TRefUnit>().OtherAcc.AsSeq4f(),
                                Vars<TSynonymUnit>().SynonymAcc.AsSeq4f()
                            ),
                            0.1f
                        )
                    );
                }
            };
        };

        UNIT(TAllUnit) {
            UNIT_STATE {
                TAccumulatorsByFloatValue AllAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, AllAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQuery& query) {
                    const size_t numWords = query.Words.size();
                    AllAcc.Init(pool, numWords);
                }
                Y_FORCE_INLINE void FinishDoc() {
                    AllAcc.CopyFrom(
                        NSeq4f::Add(
                            NSeq4f::Add(
                                Vars<TRefUnit>().ExactAcc.AsSeq4f(),
                                Vars<TRefUnit>().LemmaAcc.AsSeq4f()
                            ),
                            NSeq4f::Add(
                                Vars<TRefUnit>().OtherAcc.AsSeq4f(),
                                Vars<TSynonymUnit>().SynonymAcc.AsSeq4f()
                            )
                        )
                    );
                }
            };
        };

        UNIT(TAllFormUnit) {
            UNIT_STATE {
                TAccumulatorsByUintValue AllFormsMaskAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, AllFormsMaskAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQuery& query) {
                    const size_t numWords = query.Words.size();
                    AllFormsMaskAcc.Init(pool, numWords);
                }
                Y_FORCE_INLINE void FinishDoc() {
                    for (const size_t id : xrange(AllFormsMaskAcc.Size())) {
                        AllFormsMaskAcc[id] = Vars<TRefFormUnit>().FormsMaskAcc[id] | Vars<TSynonymFormUnit>().SynFormsMaskAcc[id];
                    }
                }
            };
        };

        template <typename M>
        using TMotor = M;
    } // MACHINE_PARTS(PositionlessProxy)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
