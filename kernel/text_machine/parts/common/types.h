#pragma once

#include "storage.h"
#include "structural_stream.h"

#include <kernel/text_machine/interface/feature.h>
#include <kernel/text_machine/interface/hit.h>
#include <kernel/text_machine/interface/query.h>
#include <kernel/text_machine/interface/query_set.h>
#include <kernel/text_machine/interface/text_machine.h>

#include <util/string/builder.h>

namespace NTextMachine {
namespace NCore {
    extern const float FloatEpsilon;

    struct THitPrecStruct {
        enum EType {
            Exact,
            Lemma,
            Other,
            HitPrecMax
        };

        static const size_t Size = HitPrecMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, Exact, Lemma, Other, HitPrecMax>();
        }
    };
    using THitPrec = NLingBoost::TEnumOps<THitPrecStruct>;
    using EHitPrecType = THitPrec::EType;

    struct THitWeightStruct {
        enum EType {
            V0,
            V1,
            V2,
            V4,
            AttenV1,
            OldTRAtten,
            TxtHead,
            TxtHiRel,
            HitWeightMax
        };

        static const size_t Size = HitWeightMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, V0, V1, V2, V4, AttenV1, OldTRAtten, TxtHead, TxtHiRel, HitWeightMax>();
        }
    };
    using THitWeight = NLingBoost::TEnumOps<THitWeightStruct>;
    using EHitWeightType = THitWeight::EType;

    struct TFormsCountStruct {
        enum EType {
            All,
            FormsCountMax
        };

        static const size_t Size = FormsCountMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, All, FormsCountMax>();
        }
    };
    using TFormsCount = NLingBoost::TEnumOps<TFormsCountStruct>;
    using EFormsCountType = TFormsCount::EType;

    struct TQueryModeStruct {
        enum EType {
            UseOriginal,
            DontUseOriginal,
            QueryModeMax
        };

        static const size_t Size = QueryModeMax;
        static TArrayRef<const EType> GetValues() {
            return ::NLingBoost::NDetail::GetStaticRegion<EType, UseOriginal, DontUseOriginal>();
        }
    };
    using TQueryMode = NLingBoost::TEnumOps<TQueryModeStruct>;
    using EQueryModeType = TQueryMode::EType;

    template <EExpansionType Expansion>
    struct TExpansionSelector {};

    template <EStreamType>
    struct TStreamSelector {};

    struct TFeatureCoords {
        size_t SlotIndex = 0; // Index of vector (slot) in features stream
        size_t FeatureIndex = 0; // Index in slot

        TFeatureCoords() = default;
        TFeatureCoords(size_t slotIndex, size_t featureIndex)
            : SlotIndex(slotIndex), FeatureIndex(featureIndex)
        {}
    };

    using TFFIdsBuffer = TPodBuffer<TFFId>;
    using TConstFFIdsBuffer = TPodBuffer<const TFFId>;

    using TFeaturesBuffer = TPodBuffer<TOptFeature>;
    using TConstFeaturesBuffer = TPodBuffer<const TOptFeature>;

    using TCoordsBuffer = TPodBuffer<const TFeatureCoords*>;
    using TConstCoordsBuffer = TPodBuffer<const TFeatureCoords* const>;

    using TFloatsBuffer = TPodBuffer<float>;
    using TConstFloatsBuffer = TPodBuffer<const float>;

    using TCountsBuffer = TPodBuffer<size_t>;
    using TConstCountsBuffer = TPodBuffer<const size_t>;

    using TFeaturesHolder = TPoolPodHolder<TOptFeature>;
    using TFloatsHolder = TPoolPodHolder<float>;
    using TCountsHolder = TPoolPodHolder<size_t>;

    using TExpansionRemap = NLingBoost::TCompactEnumRemap<TExpansion>;
    using TStreamRemap = NLingBoost::TCompactEnumRemap<TStream>;

    using TExpansionRemapView = NLingBoost::TCompactEnumRemapView<TExpansion>;
    using TStreamRemapView = NLingBoost::TCompactEnumRemapView<TStream>;

    using TOpenStreamsMap = TPoolableCompactEnumMap<TStream, bool>;
    using TStreamsMap = TPoolableCompactEnumMap<TStream, const TStream*>;
    using THitWeightsMap = TPoolableEnumMap<THitWeight, float>;

    struct TCoreSharedState {
        const TExpansionRemap& ExpansionRemap;
        const TStreamRemap& StreamRemap;
        const TStreamsMap& Streams;
        const THitWeightsMap& HitWeights;
    };
} // NCore
} // NTextMachine
