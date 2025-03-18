#pragma once

#include <nginx/modules/strm_packager/src/common/repeatable_future.h>
#include <nginx/modules/strm_packager/src/common/track_data.h>

namespace NStrm::NPackager {
    // add new track with samples contaning timed meta in ID3v2.4 format
    TRepFuture<TMediaData> AddTimedMetaTrack(
        TRequestWorker& request,
        const TIntervalP fullInterval,
        const TRepFuture<TMediaData>& media,
        const TRepFuture<TMetaData>& meta);

}
