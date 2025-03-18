#include <nginx/modules/strm_packager/src/common/dispatcher.h>
#include <library/cpp/threading/future/future.h>

// TEMP ?
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>

#include <util/generic/overloaded.h>

namespace NStrm::NPackager {
    TDispatcher::TDispatcher(
        TRequestWorker& request,
        const TConfig& config,
        const TRepFuture<TMetaData>& meta,
        ISource& mainSource)
        : Request(request)
        , MainSource(mainSource)
        , AdResolver(request, config.AdResolverConfig)
        , OrderManager(request)
    {
        meta.AddCallback(request.MakeFatalOnException(std::bind(&TDispatcher::AcceptMetaData, this, std::placeholders::_1)));
    }

    TRepFuture<TMediaData> TDispatcher::GetMediaData() {
        return OrderManager.GetData();
    }

    void TDispatcher::AcceptMetaData(const TRepFuture<TMetaData>::TDataRef& metaRef) {
        if (metaRef.Empty()) {
            Y_ENSURE(TimeEnd.Defined());
            OrderManager.SetTimeEnd(*TimeEnd);
            return;
        }

        const TMetaData& meta = metaRef.Data();

        TimeEnd = Max(meta.Interval.End, TimeEnd.GetOrElse(meta.Interval.End));

        if (!TimeBegin.Defined()) {
            TimeBegin = meta.Interval.Begin;
            OrderManager.SetTimeBegin(meta.Interval.Begin);
        }

        std::visit(
            TOverloaded{
                [&](const TMetaData::TSourceData&) {
                    OrderManager.PutData(MainSource.GetMedia(meta.Interval, Ti64TimeP(0)));
                },
                [&](const TMetaData::TAdData& data) {
                    AdResolver.Resolve(meta.Interval, data).Subscribe(Request.MakeFatalOnException(std::bind(&TDispatcher::AcceptAdResolved, this, std::placeholders::_1)));
                }},
            meta.Data);
    }

    void TDispatcher::AcceptAdResolved(const NThreading::TFuture<TAdResolver::TResult>& resFuture) {
        const TAdResolver::TResult& res = resFuture.GetValue();

        if (!res.WithAd) {
            OrderManager.PutData(MainSource.GetMedia(res.OriginalInterval, Ti64TimeP(0)));
            return;
        }

        if (res.AdInterval.Begin > res.OriginalInterval.Begin) {
            OrderManager.PutData(MainSource.GetMedia(TIntervalP{
                                                         .Begin = res.OriginalInterval.Begin,
                                                         .End = res.AdInterval.Begin,
                                                     }, Ti64TimeP(0)));
        }

        OrderManager.PutData(res.AdMedia);

        if (res.AdInterval.End < res.OriginalInterval.End) {
            OrderManager.PutData(MainSource.GetMedia(TIntervalP{
                                                         .Begin = res.AdInterval.End,
                                                         .End = res.OriginalInterval.End,
                                                     }, Ti64TimeP(0)));
        }
    }

    TAdResolver::TAdResolver(TRequestWorker& request, const TConfig& config)
        : Request(request)
        , Config(config)
    {
    }

    NThreading::TFuture<TAdResolver::TResult> TAdResolver::Resolve(TIntervalP interval, const TMetaData::TAdData& adData) {
        return GetAdSource(adData).Apply(
            [interval, adData](const TSourceFuture& future) -> TResult {
                ISource* source = future.GetValue();

                TResult result;
                result.OriginalInterval = interval;
                result.WithAd = false;

                if (!source) {
                    return result;
                }

                TIntervalP adFullInterval = source->FullInterval();
                adFullInterval.Begin += adData.BlockBegin;
                adFullInterval.End += adData.BlockBegin;

                TIntervalP intersection{
                    .Begin = Max(adFullInterval.Begin, interval.Begin),
                    .End = Min(adFullInterval.End, interval.End),
                };

                if (intersection.End > intersection.Begin) {
                    result.WithAd = true;
                    result.AdInterval = intersection;
                    result.AdMedia = source->GetMedia(
                        TIntervalP{
                            .Begin = intersection.Begin - adData.BlockBegin,
                            .End = intersection.End - adData.BlockBegin,
                        },
                        adData.BlockBegin);
                }

                return result;
            });
    }

    TSourceFuture TAdResolver::GetAdSource(const TMetaData::TAdData&) {
        // TODO: get ad source somehow
#if 0
        (void)Request;
        (void)Config;
        return NThreading::MakeFuture<ISource*>(nullptr);
#else
        // TEMP
        // return TSourceMp4File::Make(Request, Config.SourceMp4Config, "/mp4prox_aio/notexist--.mp4", "");
        return TSourceMp4File::Make(Request, Config.SourceMp4Config, "/mp4prox_aio/ad.mp4", "");
        // return TSourceMp4File::Make(Request, Config.SourceMp4Config, "/mp4prox_aio/ad_new1.mp4", "");
        // return TSourceMp4File::Make(Request, Config.SourceMp4Config, "/mp4prox_aio/source_3.mp4", "");

        // return TSourceMp4File::Make(Request, Config.SourceMp4Config, "/mp4prox_aio/video.mp4", "");

#endif
    }

    TAdResolver::TConfig::TConfig() {
    }

    TAdResolver::TConfig::TConfig(const TLocationConfig& config)
        : SourceMp4Config(config)
    {
    }

    void TAdResolver::TConfig::Check() const {
        SourceMp4Config.Check();
    }

    TDispatcher::TConfig::TConfig() {
    }

    TDispatcher::TConfig::TConfig(const TLocationConfig& config)
        : AdResolverConfig(config)
    {
    }

    void TDispatcher::TConfig::Check() const {
        AdResolverConfig.Check();
    }

}
