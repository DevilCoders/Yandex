#pragma once

#include <nginx/modules/strm_packager/src/common/order_manager.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>
#include <nginx/modules/strm_packager/src/common/source.h>

// TEMP ?
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>

#include <util/generic/queue.h>

namespace NStrm::NPackager {
    class TAdResolver {
    public:
        struct TConfig {
            TConfig();
            explicit TConfig(const TLocationConfig& config);

            void Check() const;

            TSourceMp4File::TConfig SourceMp4Config;
        };

        TAdResolver(TRequestWorker& request, const TConfig& config);

        struct TResult {
            TIntervalP OriginalInterval;
            bool WithAd;
            TIntervalP AdInterval;
            TRepFuture<TMediaData> AdMedia;
        };

        NThreading::TFuture<TResult> Resolve(TIntervalP Interval, const TMetaData::TAdData&);

    private:
        TSourceFuture GetAdSource(const TMetaData::TAdData&);

        TRequestWorker& Request;
        const TConfig Config;
    };

    class TDispatcher {
    public:
        struct TConfig {
            TConfig();
            explicit TConfig(const TLocationConfig& config);

            void Check() const;

            TAdResolver::TConfig AdResolverConfig;
        };

        TDispatcher(
            TRequestWorker& request,
            const TConfig& config,
            const TRepFuture<TMetaData>& meta,
            ISource& mainSource);

        TRepFuture<TMediaData> GetMediaData();

    private:
        void AcceptMetaData(const TRepFuture<TMetaData>::TDataRef&);
        void AcceptAdResolved(const NThreading::TFuture<TAdResolver::TResult>& resFuture);

        TRequestWorker& Request;
        ISource& MainSource;
        TMaybe<Ti64TimeP> TimeBegin;
        TMaybe<Ti64TimeP> TimeEnd;

        TAdResolver AdResolver;
        TOrderManager OrderManager;
    };
}
