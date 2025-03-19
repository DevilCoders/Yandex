#pragma once

#include <kernel/doom/standard_models/standard_models.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_io.h>

#include "ann_data_hit_adaptors.h"

namespace NDoom {

using TOffroadAnnDataDocWadIo = TOffroadDocWadIo<AnnDataIndexType, TAnnDataHit, TAnnDataHitWadVectorizer, TAnnDataHitWadSubtractor, TAnnDataHitWadPrefixVectorizer, DefaultAnnDataHitIoModel>;
using TOffroadLinkAnnDataDocWadIo = TOffroadDocWadIo<LinkAnnDataIndexType, TAnnDataHit, TAnnDataHitWadVectorizer, TAnnDataHitWadSubtractor, TAnnDataHitWadPrefixVectorizer, DefaultAnnDataHitIoModel>;
using TOffroadFactorAnnDataDocWadIo = TOffroadDocWadIo<FactorAnnDataIndexType, TAnnDataHit, TAnnDataHitWadVectorizer, TAnnDataHitWadSubtractor, TAnnDataHitWadPrefixVectorizer, DefaultAnnDataHitIoModel>;

using TOffroadFastAnnDataDocWadIo = TOffroadDocWadIo<FastAnnDataIndexType, TAnnDataHit, TAnnDataHitWadVectorizer, TAnnDataHitWadSubtractor, TAnnDataHitWithRegionWadPrefixVectorizer, DefaultAnnDataHitIoModel>;
using TOffroadImgLinkAnnDataDocWadIo = TOffroadDocWadIo<ImgLinkDataAnnDataIndexType, TAnnDataHit, TAnnDataHitWadVectorizer, TAnnDataHitWadSubtractor, TAnnDataHitWadPrefixVectorizer, DefaultAnnDataHitIoModel>;

} // namespace NDoom
