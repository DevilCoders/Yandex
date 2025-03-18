#include <nginx/modules/strm_packager/src/temp/test_mpegts_vod.h>

#include <nginx/modules/strm_packager/src/common/fragment_cutter.h>
#include <nginx/modules/strm_packager/src/common/muxer_mpegts.h>
#include <nginx/modules/strm_packager/src/common/muxer_mp4.h>
#include <nginx/modules/strm_packager/src/common/sender.h>
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>

namespace NStrm::NPackager::NTemp {
    TMpegTsVodTestWorker::TMpegTsVodTestWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ true, "packager_test_mpegts_vod:")
    {
    }

    // static
    void TMpegTsVodTestWorker::CheckConfig(const TLocationConfig& config) {
        TSourceMp4File::TConfig(config).Check();
    }

    void TMpegTsVodTestWorker::Work() {
        DisableChunkedTransferEncoding();

        const TSourceMp4File::TConfig mp4SourceConfig(Config);

        const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r_imp/source-mp3.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r_imp/source.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r_imp/idr-test.mp4", "");
        const auto muxerFuture = TMuxerMpegTs::Make(*this);
        // const auto muxerFuture = TMuxerMP4::Make(*this);
        const auto senderFuture = TSender::Make(*this);

        const auto wait = PackagerWaitExceptionOrAll(TVector{
            sourceFuture.IgnoreResult(),
            muxerFuture.IgnoreResult(),
            senderFuture.IgnoreResult(),
        });

        wait.Subscribe(MakeFatalOnException(
            [this, sourceFuture, muxerFuture, senderFuture](const NThreading::TFuture<void>& wait) {
                wait.TryRethrow();

                const auto source = sourceFuture.GetValue();
                const auto muxer = muxerFuture.GetValue();
                const auto sender = senderFuture.GetValue();

                {
                    const TIntervalP sourceFullInterval = source->FullInterval();
                    Clog << "source full interval: " << sourceFullInterval.Begin << " " << sourceFullInterval.End << Endl;
                }

                // SendData({muxer->MakeInitialSegment(source->GetTracksInfo()), false});
                // Finish();
                // return;

                muxer->SetMediaData(TFragmentCutter::Cut(source->GetMedia(TIntervalP{Ms2P(5000), Ms2P(10000)}, Ms2P(0)), Ms2P(5000000), *this));
                // muxer->SetMediaData(TFragmentCutter::Cut(source->GetMedia(TIntervalP{5000, 10000}, 0), 500, *this));
                // muxer->SetMediaData(TFragmentCutter::Cut(source->GetMedia(TIntervalP{10000, 15000}, 0), 500, *this));
                // muxer->SetMediaData(TFragmentCutter::Cut(source->GetMedia(TIntervalP{5000, 30000}, 0), 5000000, *this));
                // muxer->SetMediaData(TFragmentCutter::Cut(source->GetMedia(), 5000000, *this));
                // muxer->SetMediaData(TFragmentCutter::Cut(source->GetMedia(), 500, *this));

                sender->SetData(muxer->GetData());
            }));
    }

}
