#pragma once

#include "aggregator_types.h"

#include <kernel/text_machine/parts/common/types.h>
#include <kernel/text_machine/parts/common/seq4.h>

#include <util/generic/utility.h>
#include <util/generic/ymath.h>
#include <util/generic/ylimits.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(AggregatorArray) {
        using namespace NAggregator;

        // Messages
        //
        struct TInitInfo {
            TMemoryPool& Pool;
            size_t NumFeatures;
        };

        template <typename BufType>
        struct TAddFeaturesInfo {
            using TBuffer = BufType;

            TBuffer Features;
        };

        struct TCountFeaturesInfo {
            size_t BaseCount;
            size_t& Count;
        };

        struct TSaveFeatureIdsInfo {
            EFilterType FilterType;
            const TConstFFIdsBuffer& BaseIds;
            TFFIdsBuffer& Ids;
        };

        struct TSaveFeaturesInfo {
            EFilterType FilterType;
            TFloatsBuffer& Features;
        };

        // Accumulators
        //
        template <typename BufType,
            typename AccOpType>
        class TSimpleAccumulator
            : public NModule::TJsonSerializable
        {
        public:
            using TBufX  = BufType;
            using TAccOp = AccOpType;

            TAccOp AccOp;

        private:
            bool Initialized = false;
            TPoolPodHolder<float> Data;
            TBufX Res;

        public:
            void Init(TMemoryPool& pool, size_t count) {
                Initialized = false;
                Data.Init(pool, count, EStorageMode::Full);
                Res.Init(count);
                TAccOp::Init(Res, Data.Begin());
                // NOTE. Data array is left uninitialized
            }

            void Clear() {
                Initialized = false;
            }

            size_t Size() const {
                return Data.Count();
            }

            template <typename BufTypeY>
            void Add(const BufTypeY& rhs) {
                if (!Initialized) {
                    TAccOp::Store(Res, rhs);
                    Initialized = true;
                } else {
                    TAccOp::Store(Res, AccOp(Res, rhs));
                }
            }

            bool IsInitialized() const {
                return Initialized;
            }

            const TBufX& GetResult() const {
                return Res;
            }

            auto GetResultSeq() const
                -> decltype(TAccOp::TAccessorX::Seq(Res))
            {
                return TAccOp::TAccessorX::Seq(Res);
            }

            void SaveToJson(NJson::TJsonValue& value) const {
                if (Initialized) {
                    SAVE_JSON_VAR(value, Data);
                } else {
                    value["Data"] = NJson::TJsonValue(NJson::JSON_ARRAY);
                }
            }
        };

        class TCountAccumulator
            : public NModule::TJsonSerializable
        {
        private:
            TSingleWeightBuf Res;

        public:
            void Init(TMemoryPool& /*pool*/, size_t count) {
                Res.Init(count);
            }

            void Clear() {
                Res.SetWeight(0.0f);
            }

            size_t Size() const {
                return Res.Size();
            }

            template <typename BufTypeY>
            void Add(const BufTypeY& /*buf*/) {
                Res.Weight() += 1.0f;
            }

            bool IsInitialized() const {
                return Res.GetWeight() > 0.0f;
            }

            const TSingleWeightBuf& GetResult() const {
                return Res;
            }

            auto GetResultSeq() const
                -> decltype(Res.GetWeightsSeq())
            {
                return Res.GetWeightsSeq();
            }

            void SaveToJson(NJson::TJsonValue& value) const {
                value["Count"] = Res.GetWeight();
            }
        };

        template <typename Accumulator>
        UNIT(TAccumulatorStub) {
            UNIT_STATE {
                Accumulator Acc;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, Acc);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void Init(const TInitInfo& info) {
                    Vars().Acc.Init(info.Pool, info.NumFeatures);
                }
                Y_FORCE_INLINE void Clear() {
                    Vars().Acc.Clear();
                }
                template <typename BufType>
                Y_FORCE_INLINE void AddFeatures(const TAddFeaturesInfo<BufType>& info) {
                    Vars().Acc.Add(info.Features);
                }
            };
        };

        template <typename MasterUnitType,
            EAccumulatorType AccTypeValue>
        UNIT(TCopyAccumulatorStub) {
            using TMasterUnit = MasterUnitType;

            static Y_FORCE_INLINE TFFId PrepareId(const TFFId& id, EFilterType filterType) {
                return TFFId(id, filterType, AccTypeValue);
            }

            UNIT_STATE {
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE static void CountFeatures(const TCountFeaturesInfo& info) {
                    info.Count += info.BaseCount;
                }
                Y_FORCE_INLINE static void SaveFeatureIds(const TSaveFeatureIdsInfo& info) {
                    const size_t n = info.BaseIds.Count();

                    for (size_t i : xrange(n)) {
                        info.Ids.Cur() = PrepareId(info.BaseIds[i], info.FilterType);
                        info.Ids.Append(1);
                    }
                }
                Y_FORCE_INLINE void SaveFeatures(const TSaveFeaturesInfo& info) {
                    const auto& master = Vars<TMasterUnit>().Acc;
                    const size_t n = master.Size();

                    if (!master.IsInitialized()) {
                        for (size_t i : xrange(n)) {
                            Y_UNUSED(i);
                            info.Features.Add(0.0f);
                        }
                        return;
                    }

                    auto seq = master.GetResultSeq();
                    Y_ASSERT(seq.Avail() == n);
                    for (size_t i : xrange(n)) {
                        Y_UNUSED(i);
                        info.Features.Add(seq.Next());
                    }
                }
            };
        };

        template <typename MasterUnitType,
            typename NormUnitType,
            typename NormOpType,
            EAccumulatorType AccTypeValue,
            ENormalizerType NormTypeValue>
        UNIT(TNormAccumulatorStub) {
            using TMasterUnit = MasterUnitType;
            using TNormUnit = NormUnitType;
            using TNormOp = NormOpType;

            static Y_FORCE_INLINE TFFId PrepareId(const TFFId& id, EFilterType filterType) {
                return TFFId(id, filterType, AccTypeValue, NormTypeValue);
            }

            UNIT_STATE {
                TNormOp NormOp;
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE static void CountFeatures(const TCountFeaturesInfo& info) {
                    info.Count += info.BaseCount;
                }
                Y_FORCE_INLINE static void SaveFeatureIds(const TSaveFeatureIdsInfo& info) {
                    const size_t n = info.BaseIds.Count();

                    for (size_t i : xrange(n)) {
                        Y_UNUSED(i);
                        info.Ids.Cur() = PrepareId(info.BaseIds[i], info.FilterType);
                        info.Ids.Append(1);
                    }
                }
                Y_FORCE_INLINE void SaveFeatures(const TSaveFeaturesInfo& info) {
                    const auto& master = Vars<TMasterUnit>().Acc;
                    const auto& norm = Vars<TNormUnit>().Acc;

                    const size_t n = master.Size();

                    if (!master.IsInitialized()) {
                        for (size_t i : xrange(n)) {
                            Y_UNUSED(i);
                            info.Features.Add(0.0f);
                        }
                        return;
                    }

                    auto seq = Vars().NormOp(master.GetResult(), norm.GetResult());
                    Y_ASSERT(seq.Avail() == n);
                    float buf[4];
                    size_t i = 0;
                    for (; i + 4 < n; i += 4) {
                        TVec4f res = seq.Next4f();
                        res.Store(buf);

                        auto feat4 = info.Features.Append(4);
                        feat4[0] = buf[0];
                        feat4[1] = buf[1];
                        feat4[2] = buf[2];
                        feat4[3] = buf[3];
                    }

                    for (; i != n; ++i) {
                        info.Features.Add(seq.Next());
                    }
                }
            };
        };

        using TCountUnit = TAccumulatorStub<TCountAccumulator>;

#define UNIT_SIMPLE_ACCUMULATOR(AccName, BufType, OpType) \
    using T##AccName##Accumulator = TSimpleAccumulator<BufType, OpType>; \
    using T##AccName##Unit = TAccumulatorStub<T##AccName##Accumulator>; \

        UNIT_SIMPLE_ACCUMULATOR(SumW, TWeightsBuf, TWeightsAddOp)
        UNIT_SIMPLE_ACCUMULATOR(MaxW, TWeightsBuf, TWeightsMaxOp)
        UNIT_SIMPLE_ACCUMULATOR(MinW, TWeightsBuf, TWeightsMinOp)

        UNIT_SIMPLE_ACCUMULATOR(SumF, TFeaturesBuf, TFeaturesAddOp)
        UNIT_SIMPLE_ACCUMULATOR(MaxF, TFeaturesBuf, TFeaturesMaxOp)
        UNIT_SIMPLE_ACCUMULATOR(MinF, TFeaturesBuf, TFeaturesMinOp)

        UNIT_SIMPLE_ACCUMULATOR(SumWF, TFeaturesBuf, TFeaturesWAddOp)
        UNIT_SIMPLE_ACCUMULATOR(MaxWF, TFeaturesBuf, TFeaturesWMaxOp)
        UNIT_SIMPLE_ACCUMULATOR(MinWF, TFeaturesBuf, TFeaturesWMinOp)

        UNIT_SIMPLE_ACCUMULATOR(SumW2F, TFeaturesBuf, TFeaturesW2AddOp)

#undef UNIT_SIMPLE_ACCUMULATOR

#define UNIT_COPY_ACCUMULATOR(AccName) \
    using T##AccName##CopyUnit = TCopyAccumulatorStub<T##AccName##Unit, TAccumulator::AccName>;

        UNIT_COPY_ACCUMULATOR(MaxW)
        UNIT_COPY_ACCUMULATOR(MinW)
        UNIT_COPY_ACCUMULATOR(MaxF)
        UNIT_COPY_ACCUMULATOR(MinF)
        UNIT_COPY_ACCUMULATOR(MaxWF)
        UNIT_COPY_ACCUMULATOR(MinWF)

#undef UNIT_COPY_ACCUMULATOR

#define UNIT_NORM_ACCUMULATOR(AccName, NormName, NormOpType) \
    using T##AccName##Norm##NormName##Unit = TNormAccumulatorStub<T##AccName##Unit, \
        T##NormName##Unit, NormOpType, \
        TAccumulator::AccName, TNormalizer::NormName>;

        UNIT_NORM_ACCUMULATOR(SumF, Count, TFeaturesNormW)
        UNIT_NORM_ACCUMULATOR(SumW, Count, TWeightsNormW)

        UNIT_NORM_ACCUMULATOR(SumWF, SumW, TFeaturesNormW)
        UNIT_NORM_ACCUMULATOR(SumWF, SumF, TFeaturesNormF)

        UNIT_NORM_ACCUMULATOR(SumW2F, SumW, TFeaturesNormW)
        UNIT_NORM_ACCUMULATOR(SumW2F, SumF, TFeaturesNormF)

        UNIT_NORM_ACCUMULATOR(MaxWF, MaxW, TFeaturesNormW)
        UNIT_NORM_ACCUMULATOR(MaxWF, SumW, TFeaturesNormW)
        UNIT_NORM_ACCUMULATOR(MaxWF, MaxF, TFeaturesNormF)
        UNIT_NORM_ACCUMULATOR(MaxWF, SumF, TFeaturesNormF)

        UNIT_NORM_ACCUMULATOR(MinWF, MaxW, TFeaturesNormW)
        UNIT_NORM_ACCUMULATOR(MinWF, SumW, TFeaturesNormW)
        UNIT_NORM_ACCUMULATOR(MinWF, MaxF, TFeaturesNormF)
        UNIT_NORM_ACCUMULATOR(MinWF, SumF, TFeaturesNormF)

#undef UNIT_NORM_ACCUMULATOR

        template <typename M>
        class TMotor
            : public M
        {
        public:
            Y_FORCE_INLINE void Init(TMemoryPool& pool, size_t numFeatures) {
                TInitInfo info{pool, numFeatures};
                M::Init(info);
            }
            template <typename BufType>
            Y_FORCE_INLINE void AddFeatures(const BufType& buf) {
                TAddFeaturesInfo<BufType> info{buf};
                M::AddFeatures(info);
            }
            Y_FORCE_INLINE static size_t GetNumFeatures(size_t baseCount) {
                size_t count = 0;
                TCountFeaturesInfo info{baseCount, count};
                M::CountFeatures(info);
                return count;
            }
            Y_FORCE_INLINE static void SaveFeatureIds(EFilterType filterType,
                const TConstFFIdsBuffer& baseIds,
                TFFIdsBuffer& ids)
            {
                TSaveFeatureIdsInfo info{filterType, baseIds, ids};
                M::SaveFeatureIds(info);
            }
            Y_FORCE_INLINE void SaveFeatures(EFilterType filterType,
                TFloatsBuffer& features)
            {
                TSaveFeaturesInfo info{filterType, features};
                M::SaveFeatures(info);
            }
        };
    } // MACHINE_PARTS(AggregatorArray)
} // NCore
} // NTextMachine
