#pragma once

#include <nginx/modules/strm_packager/src/common/muxer.h>

#include <util/generic/string.h>
#include <util/generic/maybe.h>

namespace NStrm::NPackager {
    struct TCommonChunkInfo {
        ui64 ChunkIndex;
        bool IsInit;
        EContainer Container;
        TMaybe<ui32> Video;
        TMaybe<ui32> Audio;
        TMaybe<ui32> Subtitle;
        TMaybe<ui32> PartIndex;
    };

    TCommonChunkInfo ParseCommonChunkInfo(const TStringBuf chunk);
}
