#pragma once

#include "word_accumulator.h"

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(PositionlessAccumulator) {
        inline EHitPrecType GetHitPrec(EMatchPrecisionType precision) {
            if (TMatchPrecision::Exact == precision) {
                return THitPrec::Exact;
            } else if (TMatchPrecision::Lemma == precision) {
                return THitPrec::Lemma;
            } else {
                return THitPrec::Other;
            }
        }

        template <typename EntryType>
        struct TValueItems {
            EntryType& Exact;
            EntryType& Lemma;
            EntryType& Other;

            i64 ExactIndex;
            i64 LemmaIndex;
            i64 OtherIndex;

            template <typename AccessorType>
            TValueItems(AccessorType& accessor)
                : Exact(accessor.Next(ExactIndex))
                , Lemma(accessor.Next(LemmaIndex))
                , Other(accessor.Next(OtherIndex))
            {
                Y_ASSERT(ExactIndex >= 0);
                Y_ASSERT(LemmaIndex >= 0);
                Y_ASSERT(OtherIndex >= 0);
            }
        };

        template <typename EntryType>
        struct TValueFormItems {
            EntryType& FormsMask;

            i64 FormsMaskIndex;

            template <typename AccessorType>
            TValueFormItems(AccessorType& accessor)
                : FormsMask(accessor.Next(FormsMaskIndex, true))
            {
                Y_ASSERT(FormsMaskIndex >= 0);
            }
        };

        UNIT(TAccUnit) {
            UNIT_STATE {
                TAccumulatorsByFloatValue ExactAcc;
                TAccumulatorsByFloatValue LemmaAcc;
                TAccumulatorsByFloatValue OtherAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, ExactAcc);
                    SAVE_JSON_VAR(value, LemmaAcc);
                    SAVE_JSON_VAR(value, OtherAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                static void ScatterStatic(TScatterMethod::CollectValues, const TValueId& id, const TStaticValuesInfo& info) {
                    TValueItems<TValueEntry> items(info.Values);

                    items.Exact.Id = TValueId(id, THitPrec::Exact);
                    items.Lemma.Id = TValueId(id, THitPrec::Lemma);
                    items.Other.Id = TValueId(id, THitPrec::Other);

                    MACHINE_LOG("PositionlessAccumulator::TAccUnit", "CollectFloats", TStringBuilder{}
                        << "{Id: " << id.FullName() << "}");
                }

                Y_FORCE_INLINE void NewMultiQuery(TMemoryPool& pool, size_t numBlocks) {
                    ExactAcc.Init(pool, numBlocks);
                    LemmaAcc.Init(pool, numBlocks);
                    OtherAcc.Init(pool, numBlocks);
                }
                void Scatter(TScatterMethod::RegisterValues, const TValueId& id, const TRegisterValuesInfo& info) {
                    const size_t size = Vars().ExactAcc.Size();

                    TValueItems<const TValueEntry> items(info.Values);

                    Y_ASSERT(items.Exact.Id == TValueId(id, THitPrec::Exact));
                    Y_ASSERT(items.Lemma.Id == TValueId(id, THitPrec::Lemma));
                    Y_ASSERT(items.Other.Id == TValueId(id, THitPrec::Other));

                    auto& exactBuf = info.FloatsRegistry.Alloc(items.ExactIndex, size);
                    auto& lemmaBuf = info.FloatsRegistry.Alloc(items.LemmaIndex, size);
                    auto& otherBuf = info.FloatsRegistry.Alloc(items.OtherIndex, size);

                    for (size_t i : xrange(size)) {
                        exactBuf.Add(&ExactAcc[i]);
                        lemmaBuf.Add(&LemmaAcc[i]);
                        otherBuf.Add(&OtherAcc[i]);
                    }
                }
                Y_FORCE_INLINE void NewDoc() {
                    ExactAcc.Assign(0.0f);
                    LemmaAcc.Assign(0.0f);
                    OtherAcc.Assign(0.0f);
                }
                Y_FORCE_INLINE void AddHitExact(size_t blockId, size_t /*formId*/, float value) {
                    ExactAcc[blockId] += value;
                }
                Y_FORCE_INLINE void AddHitLemma(size_t blockId, size_t /*formId*/, float value) {
                    LemmaAcc[blockId] += value;
                }
                Y_FORCE_INLINE void AddHitOther(size_t blockId, size_t /*formId*/, float value) {
                    OtherAcc[blockId] += value;
                }
            };
        };

        UNIT(TFormUnit) {
            UNIT_STATE {
                TAccumulatorsByUintValue FormsMaskAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, FormsMaskAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                static void ScatterStatic(TScatterMethod::CollectValues, const TValueId& id, const TStaticValuesInfo& info) {
                    TValueFormItems<TValueEntry> items(info.Values);

                    items.FormsMask.Id = TValueId(id, TFormsCount::All);

                    MACHINE_LOG("PositionlessAccumulator::TAccUnit", "CollectFloats", TStringBuilder{}
                        << "{Id: " << id.FullName() << "}");
                }

                Y_FORCE_INLINE void NewMultiQuery(TMemoryPool& pool, size_t numBlocks) {
                    FormsMaskAcc.Init(pool, numBlocks);
                }
                void Scatter(TScatterMethod::RegisterValues, const TValueId& id, const TRegisterValuesInfo& info) {
                    const size_t size = Vars().FormsMaskAcc.Size();

                    TValueFormItems<const TValueEntry> items(info.Values);

                    Y_ASSERT(items.FormsMask.Id == TValueId(id, TFormsCount::All));

                    auto& formsMaskBuf = info.Uints64Registry.Alloc(items.FormsMaskIndex, size);

                    for (size_t i : xrange(size)) {
                        formsMaskBuf.Add(&FormsMaskAcc[i]);
                    }
                }
                Y_FORCE_INLINE void NewDoc() {
                    FormsMaskAcc.Assign(0);
                }
                Y_FORCE_INLINE void AddHitExact(const size_t blockId, const size_t formId, float /*value*/) {
                    FormsMaskAcc[blockId] |= ((ui64)1 << (formId & 63));
                }
                Y_FORCE_INLINE void AddHitLemma(const size_t blockId, const size_t formId, float /*value*/) {
                    FormsMaskAcc[blockId] |= ((ui64)1 << (formId & 63));
                }
                Y_FORCE_INLINE void AddHitOther(const size_t blockId, const size_t formId, float /*value*/) {
                    FormsMaskAcc[blockId] |= ((ui64)1 << (formId & 63));
                }
            };
        };

        template <typename M>
        class TMotor : public M {
        public:
            void NewMultiQuery(TMemoryPool& pool, size_t numBlocks) {
                M::NewMultiQuery(pool, numBlocks);
            }
            void NewDoc() {
                M::NewDoc();
            }
            void AddHit(size_t blockId, EMatchPrecisionType precision, size_t formId, float value) {
                if (THitPrec::Exact == GetHitPrec(precision)) {
                    M::AddHitExact(blockId, formId, value);
                } else if (THitPrec::Lemma == GetHitPrec(precision)) {
                    M::AddHitLemma(blockId, formId, value);
                } else {
                    M::AddHitOther(blockId, formId, value);
                }
            }
        };
    } // MACHINE_PARTS(PositionlessAccumulator)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
