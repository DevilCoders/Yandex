#pragma once

#include <nginx/modules/strm_packager/src/common/muxer.h>
#include <nginx/modules/strm_packager/src/content/common_uri.h>

#include <util/generic/string.h>
#include <util/generic/maybe.h>

namespace NStrm::NPackager {
    TSourceFuture CommonLiveSelectTracks(TRequestWorker& request, const TCommonChunkInfo& chunkInfo, TSourceFuture source);
}
