#include <nginx/modules/strm_packager/src/common/mp4_common.h>
#include <nginx/modules/strm_packager/src/common/source.h>

#include <strm/media/transcoder/mp4muxer/io.h>

namespace NStrm::NPackager {
    // static
    NThreading::TFuture<TFileMediaData> ISource::GetShmCacheFileMediaData(
        const TMaybe<TShmZone<TShmCache>>& cache,
        TRequestWorker& request,
        const TString& key,
        TShmCache::TDataGetter<TSimpleBlob> runGetter,
        const TMaybe<TIntervalP>& interval) {
        Y_ENSURE(runGetter);

        if (!cache.Defined()) {
            return runGetter().Apply([&request, interval](const NThreading::TFuture<TSimpleBlob>& future) -> TFileMediaData {
                const TBuffer savedMoov = SaveTFileMediaDataFast(future.GetValue(), request.KalturaMode);
                return LoadTFileMediaDataFromMoovData(interval, savedMoov.Data(), savedMoov.Size());
            });
        }

        return cache->Data().Get<TFileMediaData, TSimpleBlob, TBuffer>(
            request,
            key,
            runGetter,
            std::bind(LoadTFileMediaDataFromMoovData, interval, std::placeholders::_1, std::placeholders::_2),
            std::bind(SaveTFileMediaDataFast, std::placeholders::_1, request.KalturaMode));
        // just for tests: SaveTFileMediaDataSlow);
        // just for tests: SaveTFileMediaDataCheck);
    }

    TRepFuture<TMediaData> ISource::GetMedia(const TIntervalP interval, const Ti64TimeP addTsOffset) {
        const TIntervalP fullInterval = FullInterval();

        if (addTsOffset == Ti64TimeP(0) && interval == fullInterval) {
            return GetMedia();
        }

        TRepPromise<TMediaData> promise = TRepPromise<TMediaData>::Make();

        bool finished = false;

        GetMedia().AddCallback(Request.MakeFatalOnException([this, interval, addTsOffset, promise, finished](const TRepFuture<TMediaData>::TDataRef& dataPtr) mutable {
            if (dataPtr.Exception()) {
                promise.FinishWithException(dataPtr.Exception());
                return;
            }

            if (dataPtr.Empty()) {
                if (!finished) {
                    promise.Finish();
                }
                return;
            }

            const TMediaData& data = dataPtr.Data();

            TMediaData result;

            const TIntervalP toCrop = {
                .Begin = Max(interval.Begin, data.Interval.Begin),
                .End = Min(interval.End, data.Interval.End),
            };

            result.Interval.Begin = toCrop.Begin + addTsOffset;
            result.Interval.End = toCrop.End + addTsOffset;

            if (addTsOffset == Ti64TimeP(0) && toCrop == data.Interval) {
                promise.PutData(data);
            } else if (toCrop.End > toCrop.Begin) {
                result.TracksInfo = data.TracksInfo;
                result.TracksSamples.resize(data.TracksSamples.size());

                for (size_t trackIdx = 0; trackIdx < data.TracksSamples.size(); ++trackIdx) {
                    const TVector<TSampleData>& src = data.TracksSamples[trackIdx];
                    TVector<TSampleData>& dst = result.TracksSamples[trackIdx];

                    const auto srcRange = GetSamplesInterval(src, toCrop);

                    dst.reserve(srcRange.End - srcRange.Begin);

                    for (TSampleData const* s = srcRange.Begin; s != srcRange.End; ++s) {
                        dst.push_back(*s);
                        dst.back().Dts += addTsOffset;
                    }
                }

                promise.PutData(std::move(result));
            }

            if (!finished && data.Interval.End >= interval.End) {
                finished = true;
                promise.Finish();
            }
        }));

        return promise.GetFuture();
    }

    // static
    TVector<TTrackInfo const*> ISource::AllocTracksInfo(const TVector<TTrackInfo>& tracksInfo, TRequestWorker& request) {
        TVector<TTrackInfo const*> result(tracksInfo.size());

        for (size_t ti = 0; ti < tracksInfo.size(); ++ti) {
            result[ti] = request.GetPoolUtil<TTrackInfo>().New(tracksInfo[ti]);
        }

        return result;
    }

    ISource::ISource(TRequestWorker& request)
        : Request(request)
    {
    }

}
