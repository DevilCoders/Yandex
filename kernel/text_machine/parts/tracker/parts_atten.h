#pragma once

#include "parts_base.h"
#include "parts_plane.h"

#include <kernel/text_machine/parts/accumulators/word_accumulator.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        UNIT_INFO_BEGIN(TTextAttenUnit)
        UNIT_INFO_END()

        UNIT(TTextAttenUnit) {
            UNIT_STATE{
                TAccumulatorsByFloatValue FieldsAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, FieldsAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    FieldsAcc.Init(pool, info.Query.Words.size());
                }
                Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                    FieldsAcc.CopyFrom(
                        NSeq4f::Add(
                            NSeq4f::Add(
                                Vars<TPlaneAccumulatorFamily>().PlaneAccumulator.GetAtten64State().Atten64Acc.AsSeq4f(),
                                Vars<TTitlePositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f()
                            ),
                            Vars<TUrlPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(),
                            0.1f
                        )
                    );
                }

            public:
                Y_FORCE_INLINE float CalcAtten64Bm15(float k) const {
                    return FieldsAcc.CalcBm15(Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                }
            };
        };

        UNIT_INFO_BEGIN(TTextAttenExactUnit)
        UNIT_INFO_END()

        UNIT(TTextAttenExactUnit) {
            UNIT_STATE{
                TAccumulatorsByFloatValue FieldsAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, FieldsAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    FieldsAcc.Init(pool, info.Query.Words.size());
                }
                Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                    FieldsAcc.CopyFrom(
                        NSeq4f::Add(
                            NSeq4f::Add(
                                Vars<TPlaneAccumulatorFamily>().PlaneAccumulator.GetAtten64State().Atten64ExactAcc.AsSeq4f(),
                                Vars<TTitlePositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f()
                            ),
                            Vars<TUrlPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(),
                            0.1f
                        )
                    );
                }

            public:
                Y_FORCE_INLINE float CalcAtten64Bm15ExactWX(float k) const {
                    return FieldsAcc.CalcBm15(Vars<TCoreUnit>().ExactWeights.AsSeq4f(), k);
                }
            };
        };
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>

