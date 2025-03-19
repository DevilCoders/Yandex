#pragma once

#include <kernel/doom/hits/attributes_hit.h>
#include <kernel/doom/offroad/attributes_hit_adaptors.h>
#include <kernel/doom/offroad_attributes_wad/offroad_attributes_adaptors.h>
#include <kernel/doom/offroad_inv_wad/offroad_inv_wad_io.h>
#include <kernel/doom/offroad_key_wad/offroad_key_wad_io.h>
#include <kernel/doom/offroad_keyinv_wad/default_hit_range.h>
#include <kernel/doom/offroad_keyinv_wad/default_hit_range_adaptors.h>
#include <kernel/doom/offroad_keyinv_wad/default_hit_range_accessor.h>
#include <kernel/doom/offroad_keyinv_wad/offroad_keyinv_wad_io.h>

namespace NDoom {

using TAttributesHitRange = TDefaultHitRange<NOffroad::TTupleSubOffset>;
using TAttributesHitRangeVectorizer = TDefaultHitRangeVectorizer<NOffroad::TTupleSubOffset, NOffroad::TTupleSubOffsetVectorizer>;
using TAttributesHitRangeSubtractor = NOffroad::TTupleSubOffsetSubtractor;
using TAttributesHitRangeSerializer = TDefaultHitRangeSerializer<NOffroad::TTupleSubOffset, NOffroad::TTupleSubOffsetSerializer>;
using TAttributesHitRangeCombiner = TDefaultHitRangeCombiner<NOffroad::TTupleSubOffset>;
using TAttributesHitRangeAccessor = TDefaultHitRangeAccessor<NOffroad::TTupleSubOffset>;
using TAttributesSegTreeOffsetsSearcher = NOffroad::TFlatSearcher<std::nullptr_t, TAttributesHitRange, NOffroad::TNullVectorizer, TOffroadAttributesHitRangeVectorizer>;


using TOffroadAttributesKeyIo = TOffroadKeyWadIo<
    AttributesIndexType,
    TAttributesHitRange,
    TAttributesHitRangeVectorizer,
    TAttributesHitRangeSubtractor,
    TAttributesHitRangeSerializer,
    TAttributesHitRangeCombiner,
    DefaultAttributesKeyIoModel
>;

using TOffroadAttributesHitIo = TOffroadInvWadIo<
    AttributesIndexType,
    TAttributesHit,
    TAttributesHitVectorizer,
    TAttributesHitSubtractor,
    TAttributesHitVectorizer,
    DefaultAttributesHitIoModel
>;

using TOffroadAttributesIo = TOffroadKeyInvWadIo<
    TOffroadAttributesKeyIo,
    TOffroadAttributesHitIo,
    TAttributesHitRangeAccessor
>;


} // namespace NDoom
