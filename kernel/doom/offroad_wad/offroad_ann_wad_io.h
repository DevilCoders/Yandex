#pragma once

#include <library/cpp/offroad/custom/subtractors.h>

#include <kernel/doom/hits/reqbundle_hit.h>
#include <kernel/doom/hits/superlong_hit.h>
#include <kernel/doom/info/index_format.h>
#include <kernel/doom/offroad_common/reqbundle_hit_adaptors.h>
#include <kernel/doom/offroad_common/superlong_hit_adaptors.h>

#include "offroad_wad_io.h"

namespace NDoom {

using TOffroadAnnWadIoSortedMultiKeys = TOffroadWadIo<TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer, DefaultAnnSortedMultiKeysKeyIoModel, DefaultAnnSortedMultiKeysHitIoModel, true>;

using TOffroadAnnKeyInvWadIo = TOffroadWadIo<TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer, DefaultAnnSortedMultiKeysKeyIoModel, NoStandardIoModel, true, NDoom::AnnIndexType>;
using TOffroadFactorAnnKeyInvWadIo = TOffroadWadIo<TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer, DefaultAnnSortedMultiKeysKeyIoModel, NoStandardIoModel, true, NDoom::FactorAnnIndexType>;
using TOffroadLinkAnnKeyInvWadIo = TOffroadWadIo<TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer, DefaultAnnSortedMultiKeysKeyIoModel, NoStandardIoModel, true, NDoom::LinkAnnIndexType>;
using TOffroadTextKeyInvWadIo = TOffroadWadIo<TReqBundleHit, TReqBundleHitVectorizerV2, TReqBundleHitSubtractorV2, TReqBundleHitPrefixVectorizerV2, DefaultAnnSortedMultiKeysKeyIoModel, NoStandardIoModel, true, NDoom::KeyInvIndexType>;

} // namespace NDoom
