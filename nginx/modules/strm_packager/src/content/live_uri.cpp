#include <nginx/modules/strm_packager/src/content/live_uri.h>

#include <nginx/modules/strm_packager/src/common/source_tracks_select.h>

namespace NStrm::NPackager {
    TSourceFuture CommonLiveSelectTracks(TRequestWorker& request, const TCommonChunkInfo& chunkInfo, TSourceFuture source) {
        TVector<TSourceTracksSelect::TTrackFromSourceFuture> tracks;

        if (chunkInfo.Video.Defined()) {
            tracks.push_back({TSourceTracksSelect::ETrackType::Video, *chunkInfo.Video - 1, source});
        }

        if (chunkInfo.Audio.Defined()) {
            tracks.push_back({TSourceTracksSelect::ETrackType::Audio, *chunkInfo.Audio - 1, source});
        }

        if (chunkInfo.Subtitle.Defined()) {
            tracks.push_back({TSourceTracksSelect::ETrackType::Subtitle, *chunkInfo.Subtitle - 1, source});
        }

        return TSourceTracksSelect::Make(request, tracks);
    };
}
