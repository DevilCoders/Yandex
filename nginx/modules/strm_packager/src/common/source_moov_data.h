#pragma once

#include <nginx/modules/strm_packager/src/base/shm_cache.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>
#include <nginx/modules/strm_packager/src/common/track_data.h>

#include <strm/media/transcoder/mp4muxer/mux.h>

#include <library/cpp/threading/future/future.h>

namespace NStrm::NPackager {
    // media data to store in shm cache
    struct TMoovData {
        struct TTrackSamples {
            struct TDtsBlock {
                i64 DtsBegin;  // in timescale for this track
                ui32 Duration; // in timescale for this track
                ui32 IndexBegin;
            };

            struct TDataBlock {
                ui32 IndexBegin;
                ui64 DataBeginOffset;
            };

            static constexpr ui32 DataBlockMaxSamplesCount = 128;

            ui32 SamplesCount;
            ui32 Timescale;
            i32 MinCto;
            i32 MaxCto;

            TVector<TDtsBlock> DtsBlocks;
            TMaybe<TVector<i32>> Cto; // if undefined then all equal to MinCto and MaxCto
            TVector<ui32> Keyframes;  // is empty then all are keyframes
            TVector<ui32> DataSize;
            TVector<TDataBlock> DataBlocks;

            // this data is not to store
            ui32 TrackID;               // from corresponding TTrackBox
            i64 DtsOffsetByEditListBox; // offset to dts, defined by TEditListBox
        };

        TVector<TSampleData::TDataParams> TracksDataParams;
        TVector<TTrackInfo> TracksInfo;

        // samples from moov
        TVector<TTrackSamples> TracksSamples;

        // params for future moof parse
        TVector<NMP4Muxer::TTrackExtendsBox> TracksTrackExtendsBoxes;
        TVector<ui32> TracksMoofTimescale;
    };

    // media data to keep in created Source
    struct TFileMediaData {
        struct TFileSample {
            Ti64TimeP Dts;
            Ti32TimeP Cto;
            Ti32TimeP CoarseDuration;
            TSampleFlags Flags;
            ui32 DataSize;
            ui64 DataFileOffset;
            NThreading::TFuture<void> DataFuture;
            TSimpleBlob const* DataBlob;
            ui64 DataBlobOffset;
        };

        Ti64TimeP Duration;
        TVector<TSampleData::TDataParams> TracksDataParams;
        TVector<TTrackInfo> TracksInfo;

        // samples from moov
        TVector<TVector<TFileSample>> TracksSamples;

        // params for future moof parse
        TVector<NMP4Muxer::TTrackExtendsBox> TracksTrackExtendsBoxes;
        TVector<ui32> TracksMoofTimescale;
    };

    // save moov data to store in cache
    TBuffer SaveTFileMediaDataSlow(const TSimpleBlob& data, const bool kalturaMode);
    TBuffer SaveTFileMediaDataFast(const TSimpleBlob& data, const bool kalturaMode);
    TBuffer SaveTFileMediaDataCheck(const TSimpleBlob& data, const bool kalturaMode);

    // load moov data from cache
    TFileMediaData LoadTFileMediaDataFromMoovData(const TMaybe<TIntervalP>& interval, void const* buffer, size_t bufferSize);

}
