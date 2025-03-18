#pragma once

#include <nginx/modules/strm_packager/src/common/muxer.h>

#include <util/generic/string.h>
#include <util/generic/maybe.h>

namespace NStrm::NPackager {
    class TVodUri {
    public:
        struct TTrackInfo {
            TMaybe<ui32> VideoTrackId;
            TMaybe<ui32> AudioTrackId;
        };

        struct TChunkInfo {
            bool IsInitSegment;
            ui32 Number;
            EContainer Container;
        };

    public:
        explicit TVodUri(TStringBuf uri);

        TString GetDescriptionUri() const;
        const TTrackInfo& GetTrackInfo() const;
        const TChunkInfo& GetChunkInfo() const;
        TIntervalMs GetInterval(Ti64TimeMs chunkDuration) const;

    private:
        TTrackInfo ParseTracks(TStringBuf s) const;
        TChunkInfo ParseChunkInfo(TStringBuf s) const;

    private:
        TStringBuf Bucket;
        TStringBuf Directory;
        TStringBuf InputStreamID;
        TStringBuf TranscodingID;
        TStringBuf KalturaID;
        TStringBuf MetadataVersion;
        TTrackInfo TrackInfo;
        TChunkInfo ChunkInfo;
    };
}
