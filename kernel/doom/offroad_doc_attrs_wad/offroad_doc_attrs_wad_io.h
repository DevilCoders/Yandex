#pragma once

#include "doc_attrs_hit_adaptors.h"

#include <kernel/doom/standard_models/standard_models.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_io.h>

namespace NDoom {

using TOffroadDocAttrsWadIo = TOffroadDocWadIo<DocAttrsIndexType, TDocAttrsHit, TDocAttrsHitVectorizer, TDocAttrsHitSubtractor, TDocAttrsHitPrefixVectorizer, DefaultDocAttrsHitIoModel>;
using TOffroadDocAttrs64WadIo = TOffroadDocWadIo<DocAttrsIndexType, TDocAttrs64Hit, TDocAttrs64HitVectorizer, TDocAttrs64HitSubtractor, TDocAttrsHitPrefixVectorizer, DefaultDocAttrs64HitIoModel>;

} // namespace NDoom
