#pragma once

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <kernel/doom/standard_models/standard_models.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_io.h>

#include "sent_hit_adaptors.h"

namespace NDoom {

template<EWadIndexType indexType>
using TOffroadSentDocWadIo = TOffroadDocWadIo<indexType, TSentHit, TSentHitVectorizer, TSentHitSubtractor, NOffroad::TNullVectorizer, NoStandardIoModel, BitDocCodec>;

using TOffroadFactorSentDocWadIo = TOffroadSentDocWadIo<FactorSentIndexType>;
using TOffroadFastSentDocWadIo = TOffroadSentDocWadIo<FastSentIndexType>;
using TOffroadLinkSentDocWadIo = TOffroadSentDocWadIo<ImgLinkDataSentIndexType>;


template <EWadIndexType indexType, EStandardIoModel standardModel>
using TOffroadSentTypedWadIo = TOffroadDocWadIo<indexType, TSentLenHit, TSentLenHitVectorizer, NOffroad::TINSubtractor, NOffroad::TNullVectorizer, standardModel, BitDocCodec>;

using TOffroadSentWadIo = TOffroadSentTypedWadIo<SentIndexType, DefaultSentIoModel>;
using TOffroadAnnSentWadIo = TOffroadSentTypedWadIo<AnnSentIndexType, DefaultAnnSentIoModel>;
using TOffroadFactorAnnSentWadIo = TOffroadSentTypedWadIo<FactorAnnSentIndexType, DefaultFactorAnnSentIoModel>;
using TOffroadLinkAnnSentWadIo = TOffroadSentTypedWadIo<LinkAnnSentIndexType, DefaultLinkAnnSentIoModel>;


} // namespace NDoom
