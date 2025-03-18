#include <nginx/modules/strm_packager/src/temp/test_timed_meta.h>

#include <nginx/modules/strm_packager/src/common/fragment_cutter.h>
#include <nginx/modules/strm_packager/src/common/muxer_mp4.h>
#include <nginx/modules/strm_packager/src/common/muxer_mpegts.h>
#include <nginx/modules/strm_packager/src/common/sender.h>
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>
#include <nginx/modules/strm_packager/src/common/source_tracks_select.h>
#include <nginx/modules/strm_packager/src/common/source_union.h>
#include <nginx/modules/strm_packager/src/common/timed_meta.h>

namespace NStrm::NPackager::NTemp {
    TTestTimedMetaWorker::TTestTimedMetaWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ false, "packager_test_timed_meta:")
    {
    }

    // static
    void TTestTimedMetaWorker::CheckConfig(const TLocationConfig& config) {
        TSourceMp4File::TConfig(config).Check();
        TSourceUnion::TConfig(config).Check();
    }

    void TTestTimedMetaWorker::Work() {
        const TSourceMp4File::TConfig mp4SourceConfig(Config);

#if 1
        auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r_imp/source.mp4", "", TIntervalP{Ms2P(5000), Ms2P(10000)});
        sourceFuture = TSourceTracksSelect::Make(*this, {{TSourceTracksSelect::ETrackType::Video, 0, sourceFuture}});
#endif
#if 0
        auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r_imp/c4.mp4", "", TIntervalP{Ms2P(5000), Ms2P(10000)});
#endif

        const auto muxerFuture = GetArg<bool>("ts", false).GetOrElse(false) ? TMuxerMpegTs::Make(*this, /* chunkNumber = */ 1) : TMuxerMP4::Make(*this);
        const auto senderFuture = TSender::Make(*this);

        const auto wait = PackagerWaitExceptionOrAll(TVector{
            sourceFuture.IgnoreResult(),
            muxerFuture.IgnoreResult(),
            senderFuture.IgnoreResult(),
        });

        wait.Subscribe(MakeFatalOnException(
            [this, sourceFuture, muxerFuture, senderFuture](const NThreading::TFuture<void>&) {
                sourceFuture.TryRethrow();
                muxerFuture.TryRethrow();
                senderFuture.TryRethrow();

                const auto source = sourceFuture.GetValue();
                const auto muxer = muxerFuture.GetValue();
                const auto sender = senderFuture.GetValue();

                TInterval si = source->FullInterval();
                si.End = (si.End / 5000) * 5000;

                auto metaPromise = TRepPromise<TMetaData>::Make();

                // const int kn = 3;
                const int kn = 6;
                // const int kn = 30;
                for (int k = 0; k < kn; ++k) {
                    TMetaData d;
                    d.Interval.Begin = si.Begin + (si.End - si.Begin) * (k + 0) / kn;
                    d.Interval.End = si.Begin + (si.End - si.Begin) * (k + 1) / kn;

                    if (k % 2) {
                        d.Data = TMetaData::TSourceData();
                    } else {
                        d.Data = TMetaData::TAdData{
                            .BlockBegin = Ms2P(4637),
                            .BlockEnd = Ms2P(17840),
                            .Type = TMetaData::TAdData::EType::Inroll,
                            .ImpId = "some-imp-id",
                            .Id = "20210302-77504793582576",
                        };
                    }
                    metaPromise.PutData(d);
                }
                metaPromise.Finish();
                const auto meta = metaPromise.GetFuture();

                if (GetArg<bool>("init", false).GetOrElse(false)) {
                    DisableChunkedTransferEncoding();
                    SendData({muxer->MakeInitialSegment(source->GetTracksInfo()), false});
                    Finish();
                    return;
                }

                auto media = TFragmentCutter::Cut(source->GetMedia(si, Ms2P(0)), Ms2P(500), *this);
                media = AddTimedMetaTrack(*this, si, media, meta);
                muxer->SetMediaData(media);
                sender->SetData(muxer->GetData());
            }));
    }

}
