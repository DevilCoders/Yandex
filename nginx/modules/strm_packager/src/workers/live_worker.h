#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>
#include <nginx/modules/strm_packager/src/common/source_union.h>
#include <nginx/modules/strm_packager/src/common/track_data.h>
#include <nginx/modules/strm_packager/src/content/live_uri.h>

namespace NStrm::NPackager {
    class TLiveWorker: public TRequestWorker {
    public:
        TLiveWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

    private:
        class TChunkInfo: public TCommonChunkInfo {
        public:
            TString Uuid;
            TString VideoTrackName;
            TString AudioTrackName;
            bool Cmaf;

            // Video track name is specified and track id is specified
            inline bool VideoTrackRequested() const {
                return !VideoTrackName.Empty() && Video.Defined();
            }

            // Audio track name is specified and track id is specified
            inline bool AudioTrackRequestedNew() const {
                return !AudioTrackName.Empty() && Audio.Defined();
            }

            // Audio track name is not specified, but audio track id is specified. It will be taken from video source
            inline bool AudioTrackRequestedOld() const {
                return AudioTrackName.Empty() && Audio.Defined() && !VideoTrackName.Empty();
            }
        };

        void Work() override;

        TChunkInfo MakeChunkInfo() const;

        TSourceUnion::TSourceMaker CreateCompletedChunkSource(const NLiveDataProto::TStream& streamInfo, const TChunkInfo& chunkInfo, const TString& trackName);
        TSourceUnion::TSourceMaker CreateTranscoderSourceLowLatency(
            const NLiveDataProto::TStream& streamInfo,
            const TChunkInfo& chunkInfo,
            const TString& trackName,
            const Ti64TimeP chunkDuration,
            const Ti64TimeP chunkFragmentDuration);

        TMaybe<TIntervalP> ChunkInterval; // interval of full chunk
        TMaybe<TIntervalP> CutInterval;   // interval to use (can be full chunk, or part in case chunkInfo.PartIndex.Defined() )
    };
}
