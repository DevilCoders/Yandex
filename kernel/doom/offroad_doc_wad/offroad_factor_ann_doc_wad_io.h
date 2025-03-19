#pragma once

#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_io.h>

#include <kernel/doom/hits/reqbundle_hit.h>
#include <kernel/doom/offroad_common/reqbundle_hit_adaptors.h>

namespace NDoom {

using TOffroadAnnDocWadIo = TOffroadDocWadIo<AnnIndexType, TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer, DefaultAnnSortedMultiKeysHitIoModel>;
using TOffroadLinkAnnDocWadIo = TOffroadDocWadIo<LinkAnnIndexType, TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer, DefaultAnnSortedMultiKeysHitIoModel>;
using TOffroadFactorAnnDocWadIo = TOffroadDocWadIo<FactorAnnIndexType, TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer, DefaultAnnSortedMultiKeysHitIoModel>;
using TOffroadKeyInvDocWadIo = TOffroadDocWadIo<KeyInvIndexType, TReqBundleHit, TReqBundleHitVectorizerV2, TReqBundleHitSubtractorV2, TReqBundleHitPrefixVectorizerV2, KeyInvHitIoModelV2>;

} // namespace NDoom
