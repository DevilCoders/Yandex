#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/source.h>

#include <strm/media/transcoder/mp4muxer/movie.h>

namespace NStrm::NPackager {
    // this class can get single chunk/part from transcoder
    class TSourceLive: public ISource, public ISubrequestWorker {
    public:
        struct TConfig {
            TConfig();
            explicit TConfig(const TLocationConfig& locationConfig);

            void Check() const;
        };

        static TSourceFuture Make(
            TRequestWorker& request,
            const TConfig& config,
            const TString& uri,    // will be used to form utls like "uri/chunk/init-12345-1/tracks"
            const TString& tracks, // like: "[[name/]...]name"
            const ui64 chunkIndex,
            const TMaybe<ui64> partIndex,
            const Ti64TimeP chunkDuration,
            const Ti64TimeP chunkFragmentDuration);

        static TIntervalP MakeInterval(const ui64 chunkIndex, const TMaybe<ui64> partIndex, const Ti64TimeP chunkDuration, const Ti64TimeP chunkFragmentDuration) {
            TIntervalP result{
                .Begin = (chunkIndex - 1) * chunkDuration,
                .End = chunkIndex * chunkDuration};

            if (partIndex.Defined()) {
                result.Begin += chunkFragmentDuration * *partIndex;
                result.End = result.Begin + chunkFragmentDuration;
            }

            return result;
        }

        TIntervalP FullInterval() const override;
        TVector<TTrackInfo const*> GetTracksInfo() const override;
        TRepFuture<TMediaData> GetMedia() override;

    private:
        TSourceLive(
            TRequestWorker& request,
            const TString& uri,
            const TString& tracks,
            const ui64 chunkIndex,
            const TMaybe<ui64> partIndex,
            const Ti64TimeP chunkDuration,
            const Ti64TimeP chunkFragmentDuration);

        friend class TNgxPoolUtil<TSourceLive>;

        void AcceptHeaders(const THeadersOut& headers) override;
        void AcceptData(char const* const begin, char const* const end) override;
        void SubrequestFinished(const TFinishStatus status) override;

        void RequestData();

        const TIntervalP Interval;
        const Ti64TimeP FragmentDuration;

        bool Ready;
        TSourcePromise ReadyPromise;

        TVector<TTrackInfo const*> TracksInfo;
        TVector<TSampleData::TDataParams> TracksDataParams;

        TRepPromise<TMediaData> MediaPromise;
        bool FinishedWithException;

        class TFragment {
        public:
            static const size_t MinBoxSize = 16;
            TFragment(const bool withMoov, const size_t reserveBufferSize);

            char const* Append(char const* const begin, char const* const end);

            bool Ready();

            const bool WithMoov;

            NMP4Muxer::TBufferReader Data;

            bool GotFtyp;
            bool GotMoov;
            bool GotMoof;
            bool GotMdat;
            size_t DataSizeControl;

            size_t CurrentBoxStoredSize;
            size_t CurrentBoxNeedSize;
            TMaybe<size_t> CurrentBoxSize;
        };

        NMP4Muxer::TMovie Movie;
        TMaybe<TFragment> CurrentFragment;
        size_t FragmentReserveBufferSize;
        Ti64TimeP SentEnd;
        const TInstant CreationTime;
        bool FirstFragmentSent;
    };

}
