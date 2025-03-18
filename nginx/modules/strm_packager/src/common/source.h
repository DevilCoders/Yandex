#pragma once

#include <nginx/modules/strm_packager/src/base/shm_cache.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>
#include <nginx/modules/strm_packager/src/common/source_moov_data.h>
#include <nginx/modules/strm_packager/src/common/track_data.h>

#include <strm/media/transcoder/mp4muxer/mux.h>

#include <library/cpp/threading/future/future.h>

namespace NStrm::NPackager {
    // must be created like:
    // NThreading::TFuture<ISource*> MakeSource(...);
    class ISource {
    protected:
        ISource(TRequestWorker& request);

        static NThreading::TFuture<TFileMediaData> GetShmCacheFileMediaData(
            const TMaybe<TShmZone<TShmCache>>& cache,
            TRequestWorker& request,
            const TString& key,
            TShmCache::TDataGetter<TSimpleBlob> runGetter, // blob with moov box
            const TMaybe<TIntervalP>& interval);

        static TVector<TTrackInfo const*> AllocTracksInfo(const TVector<TTrackInfo>& tracksInfo, TRequestWorker& request);

        template <typename TIterator>
        auto GetSamplesInterval(TIterator begin, TIterator end, const TIntervalP interval) const;

        template <typename TContainer>
        auto GetSamplesInterval(TContainer& samples, const TIntervalP interval) const;

    protected:
        TRequestWorker& Request;

    public:
        virtual ~ISource() = default;
        virtual TIntervalP FullInterval() const = 0;
        virtual TVector<TTrackInfo const*> GetTracksInfo() const = 0;
        virtual TRepFuture<TMediaData> GetMedia() = 0;
        virtual TRepFuture<TMediaData> GetMedia(TIntervalP interval, Ti64TimeP addTsOffset);
    };

    using TSourceFuture = NThreading::TFuture<ISource*>;
    using TSourcePromise = NThreading::TPromise<ISource*>;

    inline TSourcePromise NewSourcePromise() {
        return NThreading::NewPromise<ISource*>();
    }

    template <typename TIterator>
    inline auto ISource::GetSamplesInterval(TIterator begin, TIterator end, const TIntervalP interval) const {
        return NStrm::NPackager::GetSamplesInterval(begin, end, interval, Request.KalturaMode);
    }

    template <typename TContainer>
    inline auto ISource::GetSamplesInterval(TContainer& samples, const TIntervalP interval) const {
        return NStrm::NPackager::GetSamplesInterval(samples.begin(), samples.end(), interval, Request.KalturaMode);
    }

}
