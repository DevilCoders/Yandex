#include <nginx/modules/strm_packager/src/common/source_concat.h>

namespace NStrm::NPackager {
    TSourceFuture TSourceConcat::TSourceConcat::Make(
        TRequestWorker& request,
        const TVector<TSourceFuture>& sources) {
        Y_ENSURE(!sources.empty());

        if (sources.size() == 1) {
            return sources[0];
        }

        return PackagerWaitExceptionOrAll(sources).Apply(
            [&request, sources](const NThreading::TFuture<void>& voidFuture) -> ISource* {
                voidFuture.TryRethrow();

                return request.GetPoolUtil<TSourceConcat>().New(request, sources);
            });
    }

    TSourceConcat::TSourceConcat(
        TRequestWorker& request,
        const TVector<TSourceFuture>& sources)
        : ISource(request)
    {
        Y_ENSURE(!sources.empty());
        Sources.reserve(sources.size());

        size_t tracksCount = 0;

        for (const auto& sf : sources) {
            ISource* s = sf.GetValue();
            Y_ENSURE(s);

            const TIntervalP sInterval = s->FullInterval();
            Y_ENSURE(sInterval.End > sInterval.Begin);

            if (Sources.empty()) {
                Interval = sInterval;
                tracksCount = s->GetTracksInfo().size();
            } else {
                Y_ENSURE(sInterval.Begin == Interval.End);
                Interval.End = sInterval.End;
                Y_ENSURE(s->GetTracksInfo().size() == tracksCount);
            }

            Sources.push_back(s);
        }
    }

    TIntervalP TSourceConcat::FullInterval() const {
        return Interval;
    }

    TVector<TTrackInfo const*> TSourceConcat::GetTracksInfo() const {
        return Sources.front()->GetTracksInfo();
    }

    TRepFuture<TMediaData> TSourceConcat::GetMedia() {
        return GetMedia(Interval, Ti64TimeP(0));
    }

    TRepFuture<TMediaData> TSourceConcat::GetMedia(TIntervalP interval, Ti64TimeP addTsOffset) {
        Y_ENSURE(interval.Begin >= Interval.Begin && interval.End <= Interval.End);

        TVector<TRepFuture<TMediaData>> futures;
        futures.reserve(Sources.size());

        for (const auto& s : Sources) {
            TIntervalP intervalToGet = s->FullInterval();
            intervalToGet.Begin = Max(intervalToGet.Begin, interval.Begin);
            intervalToGet.End = Min(intervalToGet.End, interval.End);

            if (intervalToGet.End <= intervalToGet.Begin) {
                continue;
            }

            futures.push_back(s->GetMedia(intervalToGet, addTsOffset));
        }

        Y_ENSURE(!futures.empty());

        return Concat(futures);
    }
}
