#include <nginx/modules/strm_packager/src/content/vod_uri.h>

#include <nginx/modules/strm_packager/src/base/http_error.h>
#include <nginx/modules/strm_packager/src/content/common_uri.h>

#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>

namespace NStrm::NPackager {
    TVodUri::TVodUri(TStringBuf uri) {
        TVector<TStringBuf> parts = StringSplitter(uri).Split('/').SkipEmpty();
        Y_ENSURE_EX(parts.size() >= 8, THttpError(404, TLOG_WARNING) << "wrong vod uri: " << uri);
        Y_ENSURE_EX(parts.size() <= 10, THttpError(404, TLOG_WARNING) << "wrong vod uri: " << uri);

        Bucket = parts[1];
        Directory = parts[2];
        InputStreamID = parts[3];
        TranscodingID = parts[4];
        KalturaID = parts[5];
        MetadataVersion = parts.size() >= 10 ? parts[6] : "desc";
        TrackInfo = ParseTracks(parts[parts.size() - 2]);
        ChunkInfo = ParseChunkInfo(parts[parts.size() - 1]);
    }

    TString TVodUri::GetDescriptionUri() const {
        TVector<TStringBuf> joinParts;
        auto descUri = TStringBuilder()
                       << Join("/", "", Bucket, Directory, InputStreamID, TranscodingID, KalturaID, MetadataVersion)
                       << ".json";
        return descUri;
    }

    TVodUri::TTrackInfo TVodUri::ParseTracks(TStringBuf s) const {
        const size_t vpos = s.find("vid");
        const size_t apos = s.find("aid");

        TVodUri::TTrackInfo info;

        if (vpos != s.npos) {
            info.VideoTrackId = 0;
            Y_ENSURE_EX(TryFromString(s.substr(vpos + 3, (apos > vpos) ? (apos - vpos - 3) : s.npos), *info.VideoTrackId), THttpError(404, TLOG_WARNING) << "videoTrackId not parsed: " << s);
        }

        if (apos != s.npos) {
            info.AudioTrackId = 0;
            Y_ENSURE_EX(TryFromString(s.substr(apos + 3, (vpos > apos) ? (vpos - apos - 3) : s.npos), *info.AudioTrackId), THttpError(404, TLOG_WARNING) << "audioTrackId not parsed: " << s);
        }

        Y_ENSURE_EX(info.AudioTrackId.Defined() || info.VideoTrackId.Defined(), THttpError(404, TLOG_WARNING) << "no audio, no video tracks: " << s);

        return info;
    }

    TVodUri::TChunkInfo TVodUri::ParseChunkInfo(TStringBuf s) const {
        const TCommonChunkInfo info = ParseCommonChunkInfo(s);

        Y_ENSURE_EX(info.Subtitle.Empty(), THttpError(404, TLOG_WARNING) << "vod uri with subtitles: " << s);
        Y_ENSURE_EX(info.PartIndex.Empty(), THttpError(404, TLOG_WARNING) << "vod uri with parts: " << s);
        Y_ENSURE_EX(info.ChunkIndex == (ui32)info.ChunkIndex, THttpError(404, TLOG_WARNING) << "vod uri too large number: " << s);
        Y_ENSURE_EX(info.Video.GetOrElse(1) == 1, THttpError(404, TLOG_WARNING) << "vod uri wrong video track index: " << s);
        Y_ENSURE_EX(info.Audio.GetOrElse(1) == 1, THttpError(404, TLOG_WARNING) << "vod uri wrong audio track index: " << s);

        return {
            .IsInitSegment = info.IsInit,
            .Number = (ui32)info.ChunkIndex,
            .Container = info.Container,
        };
    }

    const TVodUri::TTrackInfo& TVodUri::GetTrackInfo() const {
        return TrackInfo;
    }

    const TVodUri::TChunkInfo& TVodUri::GetChunkInfo() const {
        return ChunkInfo;
    }

    TIntervalMs TVodUri::GetInterval(Ti64TimeMs chunkDuration) const {
        Y_ENSURE_EX(ChunkInfo.Number > 0, THttpError(404, TLOG_WARNING) << "wrong chunk number: " << ChunkInfo.Number);
        return {
            .Begin = (ChunkInfo.Number - 1) * chunkDuration,
            .End = ChunkInfo.Number * chunkDuration,
        };
    }
}
