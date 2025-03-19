#pragma once

#include "aggregator_types.h"
#include "aggregator_array_parts.h"

#include <kernel/text_machine/parts/common/utils.h>
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
    MACHINE_PARTS(Aggregator) {
        using namespace NAggregator;

        // Messages
        //
        struct TInitInfo {
            TMemoryPool& Pool;
            size_t NumFeatures;
        };

        struct TAddFeaturesInfo {
            const TSingleWeightFeaturesBuf& FeaturesBuf;
        };

        struct TCountFeaturesInfo {
            size_t BaseCount;
            size_t& Count;
        };

        struct TSaveFeatureIdsInfo {
            const TConstFFIdsBuffer& BaseIds;
            TFFIdsBuffer& Ids;
        };

        struct TSaveFeaturesInfo {
            TFloatsBuffer& Features;
            size_t NumLines;
        };

        // Features heap
        //
        struct TFeatureKeyGen {
            float operator() (float feature, float weight) {
                return feature + 1e-4f * weight;
            }
        };

        struct TWeightKeyGen {
            float operator() (float, float weight) {
                return weight;
            }
        };

        struct TWeightedFeatureKeyGen {
            float operator() (float feature, float weight) {
                return (feature + 1e-4f) * weight;
            }
        };

        template <size_t N, typename TCompare, typename TKeyGen>
        class TFeaturesHeap
            : public NModule::TJsonSerializable
        {
        public:
            enum {
                MaxDepth = N
            };

        private:
            struct THeapElement {
                float Key;
                float Feature;
                float Weight;
                size_t Count;
            };

            struct THeapCompare {
                TCompare Compare;

                bool operator () (const THeapElement& lhs, const THeapElement& rhs) {
                    return Compare(lhs.Key, rhs.Key) || (lhs.Key == rhs.Key && lhs.Count < rhs.Count);
                }
            };

        private:
            using TScalarHeap = std::array<THeapElement, N>;
            using TVectorHeap = TPoolPodHolder<TScalarHeap>;

            size_t Size = Max<size_t>();
            size_t Count = Max<size_t>();
            size_t Depth = Max<size_t>();
            TVectorHeap Heap;

            TFloatsHolder FeaturesData;
            TFloatsHolder WeightsData;

        public:
            void Init(TMemoryPool& pool, size_t size) {
                Size = size;
                Count = 0;
                Depth = 0;
                Heap.Init(pool, size, EStorageMode::Full);
                FeaturesData.Init(pool, size, EStorageMode::Full);
                WeightsData.Init(pool, size, EStorageMode::Full);
            }

            void Clear() {
                Depth = 0;
                Count = 0;
            }

            template <typename TSeqX, typename TSeqY>
            void Update(TSeqX&& features, TSeqY&& weights) {
                Y_ASSERT(features.Avail() == Size);
                Y_ASSERT(weights.Avail() == Size);
                if (Depth < N) {
                    for (size_t i = 0; i != Size; ++i) {
                        const float feature = features.Next();
                        const float weight = weights.Next();

                        Heap[i][Depth].Key = TKeyGen()(feature, weight);
                        Heap[i][Depth].Feature = feature;
                        Heap[i][Depth].Weight = weight;
                        Heap[i][Depth].Count = Count;
                    }
                    Depth += 1;

                    if (Depth == N) {
                        for (size_t i = 0; i != Size; ++i) {
                            MakeHeap(Heap[i].begin(), Heap[i].end(), THeapCompare());
                        }
                    }
                }
                else {
                    for (size_t i = 0; i != Size; ++i) {
                        const float feature = features.Next();
                        const float weight = weights.Next();
                        const float key = TKeyGen()(feature, weight);

                        if (TCompare()(key, Heap[i].front().Key)) {
                            PopHeap(Heap[i].begin(), Heap[i].end(), THeapCompare());
                            Y_ASSERT(TCompare()(key, Heap[i].back().Key));
                            Heap[i].back().Key = key;
                            Heap[i].back().Feature = feature;
                            Heap[i].back().Weight = weight;
                            Heap[i].back().Count = Count;
                            PushHeap(Heap[i].begin(), Heap[i].end(), THeapCompare());
                        }
                    }
                }
                Count += 1;
            }

            template <typename TAcc>
            void Flush(TAcc& acc, size_t count = N) {
                Y_ASSERT(count <= N);
                TWeightsFeaturesBuf buf;
                buf.Init(Size);
                buf.SetFeatures(&FeaturesData[0]);
                buf.SetWeights(&WeightsData[0]);

                if (count < Depth) {
                    for (size_t i = 0; i != Size; ++i) {
                        Sort(Heap[i].begin(), Heap[i].begin() + Depth, THeapCompare());
                    }
                }

                size_t retCount = Min(N, Min(Depth, count));

                // NOTE. Unrolling this loop may provide
                // better performace, due to better use of
                // SSE operations (less memory buffering).
                for (size_t i = 0; i != retCount; ++i) {
                    for (size_t j = 0; j != Size; ++j) {
                        FeaturesData[j] = Heap[j][i].Feature;
                        WeightsData[j] = Heap[j][i].Weight;
                    }
                    acc.AddFeatures(buf);
                }

                Depth = 0;
            }

            size_t GetDepth() const {
                return Depth;
            }

            void SaveToJson(NJson::TJsonValue& value) const {
                SAVE_JSON_VAR(value, Size);
                SAVE_JSON_VAR(value, Count);
                SAVE_JSON_VAR(value, Depth);
            }


        };

        template <size_t N = 10>
        using TMaxFeaturesHeap = TFeaturesHeap<N, TGreater<float>, TFeatureKeyGen>;

        template <size_t N = 10>
        using TMinFeaturesHeap = TFeaturesHeap<N, TLess<float>, TFeatureKeyGen>;

        template <size_t N = 10>
        using TWeightedMaxFeaturesHeap = TFeaturesHeap<N, TGreater<float>, TWeightedFeatureKeyGen>;

        template <size_t N = 10>
        using TWeightedMinFeaturesHeap = TFeaturesHeap<N, TLess<float>, TWeightedFeatureKeyGen>;

        template <typename HeapType, EFilterType FilterType>
        struct THeapGroup {
            using THeap = HeapType;

            UNIT_FAMILY(THeapFamily)

            template <typename ArrayType>
            UNIT(THeapStub) {
                using TArray = ArrayType;
                enum {
                    ExtraFeaturesCount = 1
                };

                UNIT_FAMILY_STATE(THeapFamily) {
                    THeap Heap;
                    TArray Array;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        SAVE_JSON_VAR(value, Heap);
                        SAVE_JSON_VAR(value, Array);
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    static void CountFeatures(const TCountFeaturesInfo& info) {
                        info.Count += TArray::GetNumFeatures(info.BaseCount);
                        info.Count += ExtraFeaturesCount;
                    }
                    static void SaveFeatureIds(const TSaveFeatureIdsInfo& info) {
                        TArray::SaveFeatureIds(FilterType, info.BaseIds, info.Ids);

                        const size_t countBefore = info.Ids.Count();

                        info.Ids.Emplace(FilterType, TAlgorithm::NumX);

                        Y_ASSERT(info.Ids.Count() == countBefore + ExtraFeaturesCount);
                    }

                    Y_FORCE_INLINE void Init(const TInitInfo& info) {
                        Vars().Heap.Init(info.Pool, info.NumFeatures);
                        Vars().Array.Init(info.Pool, info.NumFeatures);
                    }
                    Y_FORCE_INLINE void Clear() {
                        Vars().Heap.Clear();
                        Vars().Array.Clear();
                    }
                    Y_FORCE_INLINE void AddFeatures(const TAddFeaturesInfo& info) {
                        Vars().Heap.Update(info.FeaturesBuf.GetFeaturesSeq(), info.FeaturesBuf.GetWeightsSeq());
                    }
                    Y_FORCE_INLINE void SaveFeatures(const TSaveFeaturesInfo& info) {
                        // FIXME. Hardcoded params for top/bottom selection.
                        // min size = 3, 10% of total lines
                        // Make it configurable.
                        size_t topSize = Min<size_t>(Vars().Heap.GetDepth(),
                                Max<size_t>(3UL, size_t(round(info.NumLines * 0.1f))));

                        Vars().Heap.Flush(Vars().Array, topSize);
                        Vars().Array.SaveFeatures(FilterType, info.Features);

                        const size_t countBefore = info.Features.Count();

                        info.Features.Add(float(topSize) / (10.0f + float(topSize)));

                        Y_ASSERT(info.Features.Count() == countBefore + ExtraFeaturesCount);
                    }
                };
            };
        };

        UNIT_FAMILY(TAllFamily);

        template <typename ArrayType>
        UNIT(TAllStub) {
            using TArray = ArrayType;

            enum {
                ExtraFeaturesCount = 5
            };

            UNIT_FAMILY_STATE(TAllFamily) {
                TArray Array;

                float SumWeights = NAN;
                float MaxWeight = NAN;
                float MinWeight = NAN;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, Array);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                static void CountFeatures(const TCountFeaturesInfo& info) {
                    info.Count += TArray::GetNumFeatures(info.BaseCount);
                    info.Count += ExtraFeaturesCount;
                }
                static void SaveFeatureIds(const TSaveFeatureIdsInfo& info) {
                    TArray::SaveFeatureIds(TFilter::All, info.BaseIds, info.Ids);

                    const size_t countBefore = info.Ids.Count();

                    info.Ids.Emplace(TFilter::All, TAlgorithm::NumX);
                    info.Ids.Emplace(TFilter::All, TAlgorithm::AvgW);
                    info.Ids.Emplace(TFilter::All, TAlgorithm::TotalW);
                    info.Ids.Emplace(TFilter::All, TAlgorithm::MaxW);
                    info.Ids.Emplace(TFilter::All, TAlgorithm::MinW);

                    Y_ASSERT(info.Ids.Count() == countBefore + ExtraFeaturesCount);
                }

                Y_FORCE_INLINE void Init(const TInitInfo& info) {
                    Vars().Array.Init(info.Pool, info.NumFeatures);
                }
                Y_FORCE_INLINE void Clear() {
                    Vars().Array.Clear();
                    Vars().SumWeights = 0.0f;
                    Vars().MaxWeight = 0.0f;
                    Vars().MinWeight = 1.0f;
                }
                Y_FORCE_INLINE void AddFeatures(const TAddFeaturesInfo& info) {
                    Vars().Array.AddFeatures(info.FeaturesBuf);
                    const float weight = info.FeaturesBuf.GetWeight();
                    Vars().SumWeights += weight;
                    Vars().MaxWeight = Max(Vars().MaxWeight, weight);
                    Vars().MinWeight = Min(Vars().MinWeight, weight);
                }
                Y_FORCE_INLINE void SaveFeatures(const TSaveFeaturesInfo& info) {
                    Vars().Array.SaveFeatures(TFilter::All, info.Features);

                    const size_t countBefore = info.Features.Count();

                    info.Features.Add(float(info.NumLines) / (10.0f + float(info.NumLines)));
                    info.Features.Add(info.NumLines > 0 ? Vars().SumWeights / float(info.NumLines) : 0.0);
                    info.Features.Add(Vars().SumWeights / (10.0f + Vars().SumWeights));
                    info.Features.Add(Vars().MaxWeight);
                    info.Features.Add(info.NumLines > 0 ? Vars().MinWeight : 0.0f);

                    Y_ASSERT(info.Features.Count() == countBefore + ExtraFeaturesCount);
                }
            };
        };

#define UNIT_AGGREGATOR_HEAP(HeapName, HeapType) \
    template <size_t Depth> \
    using T##HeapName##HeapGroup = THeapGroup<HeapType<Depth>, TFilter::HeapName>; \
    template <size_t Depth> \
    using T##HeapName##HeapFamily = typename T##HeapName##HeapGroup<Depth>::THeapFamily; \
    template <size_t Depth, typename ArrayType> \
    using T##HeapName##HeapStub = typename T##HeapName##HeapGroup<Depth>::template THeapStub<ArrayType>;

        UNIT_AGGREGATOR_HEAP(Top, TMaxFeaturesHeap)
        UNIT_AGGREGATOR_HEAP(WTop, TWeightedMaxFeaturesHeap)
        UNIT_AGGREGATOR_HEAP(Tail, TMinFeaturesHeap)
        UNIT_AGGREGATOR_HEAP(WTail, TWeightedMinFeaturesHeap)

#undef UNIT_AGGREGATOR_HEAP

        MACHINE_MOTOR {
        public:
            static size_t GetNumFeatures(size_t baseCount) {
                size_t count = 0;
                TCountFeaturesInfo info{baseCount, count};
                M::CountFeatures(info);
                return count;
            }
            static void SaveFeatureIds(const TConstFFIdsBuffer& baseIds, TFFIdsBuffer& ids) {
                const size_t countBefore = ids.Count();

                TSaveFeatureIdsInfo info{baseIds, ids};
                M::SaveFeatureIds(info);

                Y_ASSERT(countBefore + GetNumFeatures(baseIds.Count()) == ids.Count());
            }

            Y_FORCE_INLINE void Init(TMemoryPool& pool, size_t count) {
                BaseCount = count;
                TInitInfo info{pool, BaseCount};
                M::Init(info);
            }
            Y_FORCE_INLINE void Clear() {
                NumLines = 0;
                M::Clear();
            }
            Y_FORCE_INLINE void AddFeatures(const TConstFloatsBuffer& values, float weight) {
                Y_VERIFY_DEBUG(values.Count() == BaseCount,
                    "%lu != %lu", values.Count(), BaseCount);

                Y_VERIFY_DEBUG(weight >= 0.0f - FloatEpsilon, "Weight is < 0.0");
                Y_VERIFY_DEBUG(weight <= 1.0f + FloatEpsilon, "Weight is > 1.0");
                Y_VERIFY_DEBUG(!isnan(weight), "Weight is NaN");

                const float normWeight = GetNormalizedValue(weight);

                TSingleWeightFeaturesBuf buf;
                buf.Init(BaseCount);

                // const_cast can be avoided, but will requires extra buffer types
                // for const floats, probably not worth it
                buf.SetFeatures(const_cast<float*>(&values[0]));
                buf.SetWeight(normWeight);

                TAddFeaturesInfo info{buf};
                M::AddFeatures(info);

                NumLines += 1;
            }
            Y_FORCE_INLINE size_t GetNumFeatures() {
                return GetNumFeatures(BaseCount);
            }
            Y_FORCE_INLINE void SaveFeatures(TFloatsBuffer& features) {
                const size_t countBefore = features.Count();

                TSaveFeaturesInfo info{features, NumLines};
                M::SaveFeatures(info);

                Y_ASSERT(countBefore + GetNumFeatures(BaseCount) == features.Count());
            }

        private:
            size_t BaseCount = 0;
            size_t NumLines = 0;
        };
    } // MACHINE_PARTS(Aggregator)
} // NCore
} // NTextMachine
