#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/source.h>

namespace NStrm::NPackager {
    class TSourceFile: public ISource {
    public:
        struct TConfig {
            TConfig();
            explicit TConfig(const TLocationConfig& locationConfig);

            void Check() const;

            TMaybe<TShmZone<TShmCache>> MoovShmCacheZone;
            TMaybe<ui64> MaxMediaDataSubrequestSize; // same as kaltura option vod_cache_buffer_size
        };

        TSourceFile(
            TRequestWorker& request,
            const TConfig& config,
            const TFileMediaData& moov,
            const TString& uri,
            const TString& args,
            const TMaybe<TIntervalP> cropInterval,
            const TMaybe<Ti64TimeP> offset,
            const TSimpleBlob wholeFile = {});

        static TSourceFuture Make(
            TRequestWorker& request,
            const TConfig& config,
            const TString& uri,
            const TString& args,
            const TMaybe<TIntervalP> cropInterval,
            const TMaybe<Ti64TimeP> offset,
            const std::function<NThreading::TFuture<TSimpleBlob>()> moovGetter,
            const NThreading::TFuture<TSimpleBlob> wholeFile = {});

        TIntervalP FullInterval() const override;
        TVector<TTrackInfo const*> GetTracksInfo() const override;
        TRepFuture<TMediaData> GetMedia() override;
        TRepFuture<TMediaData> GetMedia(TIntervalP interval, Ti64TimeP addTsOffset) override;

    private:
        // this work as if TSourceFile::Offset is zero
        TRepFuture<TMediaData> GetMediaImpl(TIntervalP interval, Ti64TimeP addTsOffset);

        const TConfig Config;
        TFileMediaData FileMedia;
        const TVector<TTrackInfo const*> TracksInfo;
        const TString Uri;
        const TString Args;
        const TIntervalP Interval;
        const Ti64TimeP Offset;
        const TSimpleBlob WholeFile;
    };

}
