#pragma once

#include <nginx/modules/strm_packager/src/common/source.h>

namespace NStrm::NPackager {
    class TSourceTracksSelect: public ISource {
    public:
        enum class ETrackType {
            Any,
            Audio,
            Video,
            Subtitle,
        };

        struct TTrackFromSourceFuture {
            ETrackType TrackType = ETrackType::Any;
            size_t TrackIndex;
            TSourceFuture Source;
        };

        static TSourceFuture Make(
            TRequestWorker& request,
            const TVector<TTrackFromSourceFuture>& tracksSources);

        TSourceTracksSelect(
            TRequestWorker& request,
            const TVector<TTrackFromSourceFuture>& tracksSources);

    public:
        TIntervalP FullInterval() const override;
        TVector<TTrackInfo const*> GetTracksInfo() const override;
        TRepFuture<TMediaData> GetMedia() override;
        TRepFuture<TMediaData> GetMedia(TIntervalP interval, Ti64TimeP addTsOffset) override;

    private:
        struct TTrackFromSource {
            TTrackFromSource(const TTrackFromSourceFuture&);

            size_t TrackIndex;
            ISource* Source;
        };

        const TVector<TTrackFromSource> TracksSources;

        TIntervalP Interval;

        TVector<TTrackInfo const*> TracksInfo;

    private:
        class TSelector {
        public:
            struct TTrackFuture {
                size_t TrackIndex;
                TRepFuture<TMediaData> MediaFuture;
            };

            TSelector(
                TSourceTracksSelect& owner,
                const TIntervalP interval,
                const TVector<TTrackInfo const*> tracksInfo,
                const TVector<TTrackFuture>& tracks);

            TRepFuture<TMediaData> Get();

        private:
            void Accept(const TRepPromise<TMediaData>::TDataRef& dataRef, const int srcTrackIndex, const int resultTrackIndex);

            void TrySend();

            TSourceTracksSelect& Owner;

            TRepPromise<TMediaData> MediaPromise;

            TMediaData Acc;

            TVector<Ti64TimeP> TracksTsEnd;

            size_t FinishedCount;
            bool Finished;
        };
    };
}
