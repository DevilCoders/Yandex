#pragma once

#include "feature_part.h"

#include <kernel/text_machine/structured_id/full_id.h>

namespace NTextMachine {
namespace NFeatureInternals {
    struct TFeatureIdBuilder : public ::NStructuredId::TIdBuilder<EFeaturePartType> {
        using TFeaturePartsList = TListType<
            TPair<TFeaturePart::Expansion, ::NStructuredId::TPartMixin<EExpansionType>>,
            TPair<TFeaturePart::RegionClass, ::NStructuredId::TPartMixin<ERegionClassType>>,
            TPair<TFeaturePart::TrackerPrefix, ::NStructuredId::TPartMixin<ETrackerPrefixType>>,
            TPair<TFeaturePart::Filter, ::NStructuredId::TPartMixin<EFilterType>>,
            TPair<TFeaturePart::Accumulator, ::NStructuredId::TPartMixin<EAccumulatorType>>,
            TPair<TFeaturePart::Normalizer, ::NStructuredId::TPartMixin<ENormalizerType>>,
            TPair<TFeaturePart::Stream, ::NStructuredId::TPartMixin<TStreamSet>>,
            TPair<TFeaturePart::StreamIndex, ::NStructuredId::TPartMixin<TStreamIndexValue>>,
            TPair<TFeaturePart::StreamValue, ::NStructuredId::TPartMixin<EStreamValueType>>,
            TPair<TFeaturePart::Algorithm, ::NStructuredId::TPartMixin<EAlgorithmType>>,
            TPair<TFeaturePart::KValue, ::NStructuredId::TPartMixin<TKValue>>,
            TPair<TFeaturePart::NormValue, ::NStructuredId::TPartMixin<TNormValue>>,
            TPair<TFeaturePart::RawId, ::NStructuredId::TPartMixin<TStringBuf>>
        >;
    };
} // NFeatureInternals
} // NTextMachine

namespace NStructuredId {
    template <>
    struct TGetIdTraitsList<NTextMachine::EFeaturePartType> {
        using TResult = NTextMachine::NFeatureInternals::TFeatureIdBuilder::TFeaturePartsList;
    };
} // NStructuredId
