#include <nginx/modules/strm_packager/src/common/source_tracks_select.h>

namespace NStrm::NPackager {
    // static
    TSourceFuture TSourceTracksSelect::Make(
        TRequestWorker& request,
        const TVector<TTrackFromSourceFuture>& tracksSources) {
        Y_ENSURE(!tracksSources.empty());

        TVector<TSourceFuture> allSources;
        allSources.reserve(tracksSources.size());
        for (const auto& ts : tracksSources) {
            allSources.push_back(ts.Source);
        }

        return PackagerWaitExceptionOrAll(allSources).Apply([&request, tracksSources](const NThreading::TFuture<void>& voidFuture) -> ISource* {
            voidFuture.TryRethrow();

            return request.GetPoolUtil<TSourceTracksSelect>().New(request, tracksSources);
        });
    }

    TSourceTracksSelect::TTrackFromSource::TTrackFromSource(const TTrackFromSourceFuture& ts) {
        Y_ENSURE(ts.Source.HasValue());
        Source = ts.Source.GetValue();
        Y_ENSURE(Source);

        const auto sourceTracksInfo = Source->GetTracksInfo();
        if (ts.TrackType == ETrackType::Any) {
            TrackIndex = ts.TrackIndex;
        } else {
            bool found = false;
            size_t videoIndex = 0;
            size_t audioIndex = 0;
            size_t subtitleIndex = 0;

            for (size_t i = 0; i < sourceTracksInfo.size(); ++i) {
                const bool isVideo = std::holds_alternative<TTrackInfo::TVideoParams>(sourceTracksInfo[i]->Params);
                const bool isAudio = std::holds_alternative<TTrackInfo::TAudioParams>(sourceTracksInfo[i]->Params);
                const bool isSubtitle = std::holds_alternative<TTrackInfo::TSubtitleParams>(sourceTracksInfo[i]->Params);

                if (
                    (isSubtitle && ts.TrackType == ETrackType::Subtitle && subtitleIndex == ts.TrackIndex) ||
                    (isVideo && ts.TrackType == ETrackType::Video && videoIndex == ts.TrackIndex) ||
                    (isAudio && ts.TrackType == ETrackType::Audio && audioIndex == ts.TrackIndex))
                {
                    TrackIndex = i;
                    found = true;
                    break;
                }

                if (isAudio) {
                    ++audioIndex;
                } else if (isVideo) {
                    ++videoIndex;
                } else if (isSubtitle) {
                    ++subtitleIndex;
                }
            }

            Y_ENSURE(found);
        }
        Y_ENSURE(TrackIndex < sourceTracksInfo.size());
    }

    TSourceTracksSelect::TSourceTracksSelect(
        TRequestWorker& request,
        const TVector<TTrackFromSourceFuture>& tracksSources)
        : ISource(request)
        , TracksSources(tracksSources.begin(), tracksSources.end())
    {
        Y_ENSURE(!TracksSources.empty());

        Interval = TracksSources.front().Source->FullInterval();

        bool allEmpty = true;

        TracksInfo.reserve(TracksSources.size());
        for (const TTrackFromSource& tfs : TracksSources) {
            const auto sourceTracksInfo = tfs.Source->GetTracksInfo();

            Y_ENSURE(tfs.TrackIndex < sourceTracksInfo.size());
            TracksInfo.push_back(sourceTracksInfo[tfs.TrackIndex]);

            TIntervalP sourceInterval = tfs.Source->FullInterval();

            Interval.Begin = Max(Interval.Begin, sourceInterval.Begin);
            Interval.End = Min(Interval.End, sourceInterval.End);

            allEmpty = allEmpty && (sourceInterval.End == sourceInterval.Begin);
        }

        Y_ENSURE(allEmpty || Interval.End > Interval.Begin);

        if (allEmpty) {
            Interval = TIntervalP();
        }
    }

    TIntervalP TSourceTracksSelect::FullInterval() const {
        return Interval;
    }

    TVector<TTrackInfo const*> TSourceTracksSelect::GetTracksInfo() const {
        return TracksInfo;
    }

    TRepFuture<TMediaData> TSourceTracksSelect::GetMedia() {
        return GetMedia(FullInterval(), Ti64TimeP(0));
    }

    TRepFuture<TMediaData> TSourceTracksSelect::GetMedia(TIntervalP interval, Ti64TimeP addTsOffset) {
        TVector<TSelector::TTrackFuture> tracks;
        tracks.reserve(TracksSources.size());

        for (const auto& ts : TracksSources) {
            tracks.push_back({
                ts.TrackIndex,
                ts.Source->GetMedia(interval, addTsOffset),
            });
        }

        return Request.GetPoolUtil<TSelector>().New(*this, TIntervalP{interval.Begin + addTsOffset, interval.End + addTsOffset}, TracksInfo, tracks)->Get();
    }

    TSourceTracksSelect::TSelector::TSelector(
        TSourceTracksSelect& owner,
        const TIntervalP interval,
        const TVector<TTrackInfo const*> tracksInfo,
        const TVector<TTrackFuture>& tracks)
        : Owner(owner)
        , MediaPromise(TRepPromise<TMediaData>::Make())
        , FinishedCount(0)
        , Finished(false)
    {
        Acc.Interval = interval;
        Acc.TracksInfo = tracksInfo;
        Acc.TracksSamples.resize(tracksInfo.size());

        TracksTsEnd.assign(tracksInfo.size(), interval.Begin);

        for (size_t resultTrackIndex = 0; resultTrackIndex < tracks.size(); ++resultTrackIndex) {
            const auto& tf = tracks[resultTrackIndex];
            tf.MediaFuture.AddCallback(Owner.Request.MakeFatalOnException(std::bind(&TSelector::Accept, this, std::placeholders::_1, tf.TrackIndex, resultTrackIndex)));
        }
    }

    TRepFuture<TMediaData> TSourceTracksSelect::TSelector::Get() {
        return MediaPromise.GetFuture();
    }

    void TSourceTracksSelect::TSelector::Accept(
        const TRepPromise<TMediaData>::TDataRef& dataRef,
        const int srcTrackIndex,
        const int resultTrackIndex) try {
        if (Finished) {
            return;
        }

        if (dataRef.Empty()) {
            ++FinishedCount;
        } else {
            const TMediaData& data = dataRef.Data();
            Y_ENSURE(data.Interval.Begin >= TracksTsEnd[resultTrackIndex]);

            const TVector<TSampleData>& src = data.TracksSamples[srcTrackIndex];
            TVector<TSampleData>& dst = Acc.TracksSamples[resultTrackIndex];

            dst.insert(dst.end(), src.begin(), src.end());

            TracksTsEnd[resultTrackIndex] = data.Interval.End;
        }

        TrySend();

    } catch (const TFatalExceptionContainer&) {
        throw;
    } catch (...) {
        Finished = true;
        MediaPromise.FinishWithException(std::current_exception());
    }

    void TSourceTracksSelect::TSelector::TrySend() {
        if (FinishedCount == TracksTsEnd.size()) {
            Finished = true;
            MediaPromise.Finish();
            return;
        }

        const Ti64TimeP minEnd = *MinElement(TracksTsEnd.begin(), TracksTsEnd.end());

        Y_ENSURE(minEnd >= Acc.Interval.Begin);
        if (minEnd == Acc.Interval.Begin) {
            return;
        }

        TMediaData result;
        result.Interval.Begin = Acc.Interval.Begin;
        result.Interval.End = minEnd;
        result.TracksInfo = Acc.TracksInfo;
        result.TracksSamples.resize(TracksTsEnd.size());

        for (size_t trackIndex = 0; trackIndex < TracksTsEnd.size(); ++trackIndex) {
            TVector<TSampleData>& src = Acc.TracksSamples[trackIndex];
            TVector<TSampleData>& dst = result.TracksSamples[trackIndex];

            const auto range = Owner.GetSamplesInterval(src.begin(), src.end(), result.Interval);
            if (!range.End) {
                continue;
            }

            const auto rangeEnd = src.begin() + (range.End - &src.front());
            dst.assign(src.begin(), rangeEnd);
            src.erase(src.begin(), rangeEnd);
        }

        Acc.Interval.Begin = minEnd;

        MediaPromise.PutData(result);
    }

}
