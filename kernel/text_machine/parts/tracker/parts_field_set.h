#pragma once

#include "parts_base.h"
#include "positionless_units.h"
#include "annotation_units.h"
#include "parts_field_set_extractors.h"

#include <kernel/text_machine/parts/accumulators/word_accumulator.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        UNIT_INFO_BEGIN(TFieldSet1Unit)
        UNIT_INFO_END()

        UNIT(TFieldSet1Unit)
        {
            UNIT_STATE {
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
                            NSeq4f::Log(Vars<TUrlPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f),
                            NSeq4f::Log(Vars<TTitlePositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TBodyPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f),
                            NSeq4f::Log(Vars<TLinksPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TCorrectedCtrPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f),
                            NSeq4f::Log(Vars<TLongClickPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f)
                        )
                    );

                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TOneClickPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f),
                            NSeq4f::Log(Vars<TBrowserPageRankPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TSplitDwellTimePositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f),
                            NSeq4f::Log(Vars<TSamplePeriodDayFrcPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Add(
                                NSeq4f::Log(Vars<TSimpleClickPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f),
                                NSeq4f::Log(Vars<TYabarVisitsPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f)
                            ),
                            NSeq4f::Log(Vars<TYabarTimePositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f)
                        )
                    );
                }

            public:
                Y_FORCE_INLINE float CalcBm15FLog(float k) const {
                    return FieldsAcc.CalcBm15(Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                }
            };
        };

        UNIT_INFO_BEGIN(TFieldSet2Unit)
        UNIT_INFO_END()

        UNIT(TFieldSet2Unit)
        {
            UNIT_STATE {
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
                            NSeq4f::Log(Vars<TUrlPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.5f),
                            NSeq4f::Log(Vars<TTitlePositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TBodyPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.1f),
                            NSeq4f::Log(Vars<TCorrectedCtrPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.01f)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TLongClickPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.1f),
                            NSeq4f::Log(Vars<TOneClickPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.1f)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TBrowserPageRankPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.01f),
                            NSeq4f::Log(Vars<TSplitDwellTimePositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.01f)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TSamplePeriodDayFrcPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.01f),
                            NSeq4f::Log(Vars<TSimpleClickPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.01f)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TYabarVisitsPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.01f),
                            NSeq4f::Log(Vars<TYabarTimePositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.01f)
                        )
                    );
                }

            public:
                Y_FORCE_INLINE float CalcBm15FLog(float k) const {
                    return FieldsAcc.CalcBm15(Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                }
            };
        };

        UNIT_INFO_BEGIN(TFieldSet3Unit)
        UNIT_INFO_END()

        UNIT(TFieldSet3Unit)
        {
            UNIT_STATE {
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
                    if (FieldsAcc.Size() > 1) {
                        FieldsAcc.CopyFrom(
                            NSeq4f::Add(
                                NSeq4f::Log(Vars<TTitleAnnotationFamily>().AnnotationAccumulator.GetAnyBclmState().IdfValues.AsSeq4f(), 1.0f),
                                NSeq4f::Log(Vars<TBodyAnnotationFamily>().AnnotationAccumulator.GetAnyBclmState().IdfValues.AsSeq4f(), 0.05f)
                            )
                        );
                        FieldsAcc.AddFrom(
                            NSeq4f::Add(
                                NSeq4f::Add(
                                    NSeq4f::Log(Vars<TLongClickAnnotationFamily>().AnnotationAccumulator.GetAnyBclmState().IdfValues.AsSeq4f(), 0.1f),
                                    NSeq4f::Log(Vars<TLongClickSPAnnotationFamily>().AnnotationAccumulator.GetAnyBclmState().IdfValues.AsSeq4f(), 0.01f)
                                ),
                                NSeq4f::Log(Vars<TOneClickAnnotationFamily>().AnnotationAccumulator.GetAnyBclmState().IdfValues.AsSeq4f(), 0.2f)
                            )
                        );
                    } else {
                        FieldsAcc.CopyFrom(
                            NSeq4f::Add(
                                NSeq4f::Log(Vars<TTitlePositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 1.0f),
                                NSeq4f::Log(Vars<TBodyPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.05f)
                            )
                        );
                        FieldsAcc.AddFrom(
                            NSeq4f::Add(
                                NSeq4f::Add(
                                    NSeq4f::Log(Vars<TLongClickPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.1f),
                                    NSeq4f::Log(Vars<TLongClickSPPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.01f)
                                ),
                                NSeq4f::Log(Vars<TOneClickPositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(), 0.2f)
                            )
                        );
                    }
                }

            public:
                Y_FORCE_INLINE float CalcBclmWeightedFLogW0(float k) const {
                    return FieldsAcc.CalcBm15W0(k);
                }
            };
        };

        UNIT_INFO_BEGIN(TFieldSet4Unit)
        UNIT_INFO_END()

        UNIT(TFieldSet4Unit)
        {
            UNIT_STATE {
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
                            NSeq4f::Log(Vars<TUrlPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.1),
                            NSeq4f::Log(Vars<TTitlePositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 1.0)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TBodyPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.1),
                            NSeq4f::Log(Vars<TCorrectedCtrPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.1)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TLongClickPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.01),
                            NSeq4f::Log(Vars<TOneClickPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.01)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TBrowserPageRankPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.01),
                            NSeq4f::Log(Vars<TSplitDwellTimePositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.01)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TSamplePeriodDayFrcPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.01),
                            NSeq4f::Log(Vars<TSimpleClickPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.1)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TYabarVisitsPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.001),
                            NSeq4f::Log(Vars<TYabarTimePositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.001)
                        )
                    );
                }

            public:
                Y_FORCE_INLINE float CalcBm15FLogExactWX(float k) const {
                    return FieldsAcc.CalcBm15(Vars<TCoreUnit>().ExactWeights.AsSeq4f(), k);
                }
            };
        };

        UNIT_INFO_BEGIN(TFieldSet5Unit)
        UNIT_INFO_END()

        UNIT(TFieldSet5Unit)
        {
            UNIT_STATE {
                TAccumulatorsByFloatValue FieldsAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, FieldsAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    FieldsAcc.Init(pool, (info.Query.Words.size() < 3) ? 1 : ((info.Query.Words.size()) - 2));
                }
                Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                    //0.85 const was selected assumed that 90% of LongClickSP values < 0.85
                    //So single trigram value will be 1.0 if trigram is in title
                    //trigram value will be >=0.85 if trigram is in body or there is good annotation with trigram
                    //else trigram value is taken from max value LongClickSP annotation with trigram
                    FieldsAcc.CopyFrom(
                        NSeq4f::Max(
                            NSeq4f::Max(
                                Vars<TTitleAnnotationFamily>().AnnotationAccumulator.GetAnyTrigramState().MaxAcc.AsSeq4f(),
                                NSeq4f::Mul(Vars<TBodyAnnotationFamily>().AnnotationAccumulator.GetAnyTrigramState().MaxAcc.AsSeq4f(), 0.85f)
                            ),
                            Vars<TLongClickSPAnnotationFamily>().AnnotationAccumulator.GetAnyTrigramState().MaxAcc.AsSeq4f()
                        )
                    );
                }

            public:
                Y_FORCE_INLINE float CalcAvgPerTrigramMaxValueAny() const {
                    Y_ASSERT(FieldsAcc.Size() > 0);
                    float result = 0.0;
                    for (size_t i = 0; i < FieldsAcc.Size(); ++i) {
                        result += FieldsAcc[i];
                    }
                    return result / float(FieldsAcc.Size());
                }
            };
        };

        UNIT_INFO_BEGIN(TFieldSetUTUnit)
        UNIT_INFO_END()

        UNIT(TFieldSetUTUnit)
        {
            UNIT_STATE {
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
                            NSeq4f::Log(Vars<TUrlPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 1.0),
                            NSeq4f::Log(Vars<TUrlPositionlessFamily>().PositionlessProxy.AllAcc.AsSeq4f(), 0.01)
                        )
                    );
                    FieldsAcc.AddFrom(
                        NSeq4f::Add(
                            NSeq4f::Log(Vars<TTitlePositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f(), 0.1),
                            NSeq4f::Log(Vars<TTitlePositionlessFamily>().PositionlessProxy.AllAcc.AsSeq4f(), 0.001)
                        )
                    );

                }

            public:
                Y_FORCE_INLINE float CalcBm15FLogW0(float k) const {
                    return FieldsAcc.CalcBm15W0(k);
                }
            };
        };

        UNIT_INFO_BEGIN(TBclmMixUnit, EStreamType /*stream*/)
        UNIT_INFO_END()

        template <EStreamType Stream, EHitWeightType Weight = THitWeight::V1>
        UNIT(TBclmMixUnit)
        {
            UNIT_STATE {
                TAccumulatorsByFloatValue BclmMixAcc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, BclmMixAcc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                using AnnotationFamily = typename TAnnotationGroup<Stream>::TAnnotationFamily;
                using PositionlessFamily = typename TPositionlessGroup<Weight, Stream>::TPositionlessFamily;

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    Vars().BclmMixAcc.Init(pool, info.Query.Words.size());
                }

                Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                    const float alpha = 0.01f;
                    Vars().BclmMixAcc.CopyFrom(
                        NSeq4f::LinearMix(
                            Vars<AnnotationFamily>().AnnotationAccumulator.GetAnyBclmState().WordValues.AsSeq4f(),
                            Vars<PositionlessFamily>().PositionlessProxy.WeightedAcc.AsSeq4f(),
                            alpha)
                    );

                }
            public:
                Y_FORCE_INLINE float CalcBclmMixPlain(float k) const {
                    return Vars().BclmMixAcc.CalcBm15W0(k);
                }
            };
        };

        UNIT_INFO_BEGIN(TBclmSize1Unit, EStreamType /*stream*/)
        UNIT_INFO_END()

        // Family of factors that calcs some kind of bclm if query has more than one word,
        //    otherwise calcs bm15 over form-value accumulator
        template <EStreamType Stream, EHitWeightType HitWeightType = EHitWeightType::V1>
        UNIT(TBclmSize1Unit) {
            UNIT_STATE {
                void SaveToJson(NJson::TJsonValue& value) const {
                    Y_UNUSED(value);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                using AnnotationFamily = typename TAnnotationGroup<Stream>::TAnnotationFamily;
                using PositionlessFamily = typename TPositionlessGroup<HitWeightType, Stream>::TPositionlessFamily;

            public:
                Y_FORCE_INLINE float CalcBclmPlaneProximity1Bm15W0Size1(float k) const {
                    if (Vars<TCoreUnit>().Query->Words.size() == 1) {
                        return Vars<PositionlessFamily>().PositionlessProxy.WeightedAcc
                            .CalcBm15W0(k);
                    } else {
                        return Vars<AnnotationFamily>().AnnotationAccumulator.GetAnyBclmState().WordValues
                            .CalcBm15W0(k);
                    }
                }
                Y_FORCE_INLINE float CalcBclmWeightedProximity1Bm15Size1(float k) const {
                    if (Vars<TCoreUnit>().Query->Words.size() == 1) {
                        return Vars<PositionlessFamily>().PositionlessProxy.WeightedAcc
                            .CalcBm15W0(k);
                    } else {
                        return Vars<AnnotationFamily>().AnnotationAccumulator.GetAnyBclmState().IdfValues
                            .CalcBm15(Vars<TCoreUnit>().MainWeights.AsSeq4f(), k);
                    }
                }
            };
        };
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
