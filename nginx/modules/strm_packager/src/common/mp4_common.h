#pragma once

#include <nginx/modules/strm_packager/src/common/avcC_box_util.h>
#include <nginx/modules/strm_packager/src/common/hvcC_box_util.h>
#include <nginx/modules/strm_packager/src/common/source.h>
#include <nginx/modules/strm_packager/src/common/track_data.h>

#include <nginx/modules/strm_packager/src/base/workers.h>

namespace NStrm::NPackager {
    TSampleData::TDataParams Mp4TrackParams2TrackDataParams(const TTrackInfo::TParams& params);

    TMoovData ReadMoov(const NMP4Muxer::TMovieBox& moov, const bool kalturaMode);
}
