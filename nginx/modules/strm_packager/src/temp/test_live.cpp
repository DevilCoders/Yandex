#include <nginx/modules/strm_packager/src/temp/test_live.h>

#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/common/dispatcher.h>
#include <nginx/modules/strm_packager/src/common/fragment_cutter.h>
#include <nginx/modules/strm_packager/src/common/muxer_mp4.h>
#include <nginx/modules/strm_packager/src/common/sender.h>
#include <nginx/modules/strm_packager/src/common/source_live.h>
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>
#include <nginx/modules/strm_packager/src/common/source_union.h>

namespace NStrm::NPackager::NTemp {
    // static
    void TLiveTestWorker::CheckConfig(const TLocationConfig& config) {
        TSourceUnion::TConfig(config).Check();
        TSourceLive::TConfig(config).Check();
        TSourceMp4File::TConfig(config).Check();
        TDispatcher::TConfig(config).Check();
    }

    TLiveTestWorker::TLiveTestWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ false, "packager_test_live:")
    {
    }

    void TLiveTestWorker::Work() {
        const Ti64TimeMs chunkDurationMs(10000);
        const ui64 chunkIndex = Now().MilliSeconds() / chunkDurationMs.Value;
        const TMaybe<ui64> partIndex = {};

        const Ti64TimeP chunkDuration(chunkDurationMs);
        const Ti64TimeP chunkFragmentDuration = Ms2P(500);

        const TSourceLive::TConfig liveConfig(Config);
        const TSourceMp4File::TConfig mp4Config(Config);
        const TSourceUnion::TConfig unionConfig(Config);
        const TDispatcher::TConfig dispatcherConfig(Config);

        liveConfig.Check();
        mp4Config.Check();
        unionConfig.Check();
        dispatcherConfig.Check();

        const TInterval chunkInterval = TSourceLive::MakeInterval(chunkIndex, partIndex, chunkDuration, chunkFragmentDuration);

        const auto mainSourceMaker = [this, chunkIndex, partIndex, chunkDuration, chunkFragmentDuration, liveConfig]() -> TSourceFuture {
            Clog << "\n\n making main source \n\n";
            return TSourceLive::Make(*this, liveConfig, "/localtrnsng1", "169_240p", chunkIndex, partIndex, chunkDuration, chunkFragmentDuration);
        };

        const auto backupSourceMaker = [this, chunkIndex, partIndex, chunkDuration, chunkFragmentDuration, liveConfig]() -> TSourceFuture {
            Clog << "\n\n making backup source \n\n";
            return TSourceLive::Make(*this, liveConfig, "/localtrnsng2", "169_240p", chunkIndex, partIndex, chunkDuration, chunkFragmentDuration);
        };

        const auto stubSourceMaker = [this, chunkInterval, mp4Config]() -> TSourceFuture {
            Clog << "\n\n making stub source \n\n";
            return TSourceMp4File::Make(*this, mp4Config, "/mp4prox_aio/stub.mp4", "", TIntervalP{Ti64TimeP(0), chunkInterval.End - chunkInterval.Begin}, chunkInterval.Begin);
        };

        const auto sourceFuture = TSourceUnion::Make(
            *this,
            unionConfig,
            {
                mainSourceMaker,
                backupSourceMaker,
                stubSourceMaker,
            });

        const auto muxerFuture = TMuxerMP4::Make(*this);
        const auto senderFuture = TSender::Make(*this);

        const auto wait = PackagerWaitExceptionOrAll(TVector{
            sourceFuture.IgnoreResult(),
            muxerFuture.IgnoreResult(),
            senderFuture.IgnoreResult(),
        });

        wait.Subscribe(MakeFatalOnException(
            [this, sourceFuture, muxerFuture, senderFuture, dispatcherConfig, chunkFragmentDuration](const NThreading::TFuture<void>&) {
                sourceFuture.TryRethrow();
                muxerFuture.TryRethrow();
                senderFuture.TryRethrow();

                const auto source = sourceFuture.GetValue();
                const auto muxer = muxerFuture.GetValue();
                const auto sender = senderFuture.GetValue();

                {
                    const TInterval sourceFullInterval = source->FullInterval();
                    Clog << "live source full interval: " << sourceFullInterval.Begin << " " << sourceFullInterval.End << Endl;
                }

                const TInterval si = source->FullInterval();

                auto metaPromise = TRepPromise<TMetaData>::Make();

#if 1
                metaPromise.PutData(TMetaData{
                    .Interval = si,
                    .Data = TMetaData::TSourceData(),
                });
#else
                // just for test
                const int kn = 5;
                for (int k = 0; k < kn; ++k) {
                    TMetaData d;
                    d.Interval.Begin = si.Begin + (si.End - si.Begin) * (k + 0) / kn;
                    d.Interval.End = si.Begin + (si.End - si.Begin) * (k + 1) / kn;

                    if (k % 2) {
                        d.Data = TMetaData::TSourceData();
                    } else {
                        d.Data = TMetaData::TAdData{.AdBeginMs = (i64)si.Begin};
                    }
                    metaPromise.PutData(d);
                }
#endif
                metaPromise.Finish();
                const auto meta = metaPromise.GetFuture();

                if (GetArg<bool>("init", false).GetOrElse(false)) {
                    DisableChunkedTransferEncoding();
                    SendData({muxer->MakeInitialSegment(source->GetTracksInfo()), false});
                    Finish();
                    return;
                }

                const auto dispatcher = GetPoolUtil<TDispatcher>().New(*this, dispatcherConfig, meta, *source);

                muxer->SetMediaData(TFragmentCutter::Cut(dispatcher->GetMediaData(), chunkFragmentDuration, *this));

                sender->SetData(muxer->GetData());
            }));
    }

}
