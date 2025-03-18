#include <nginx/modules/strm_packager/src/temp/test_mp4_vod.h>

#include <nginx/modules/strm_packager/src/common/convert_subtitles_raw_to_ttml.h>
#include <nginx/modules/strm_packager/src/common/dispatcher.h>
#include <nginx/modules/strm_packager/src/common/fragment_cutter.h>
#include <nginx/modules/strm_packager/src/common/muxer_mp4.h>
#include <nginx/modules/strm_packager/src/common/muxer_webvtt.h>
#include <nginx/modules/strm_packager/src/common/sender.h>
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>
#include <nginx/modules/strm_packager/src/common/source_tracks_select.h>
#include <nginx/modules/strm_packager/src/common/source_union.h>
#include <nginx/modules/strm_packager/src/common/source_webvtt_file.h>

namespace NStrm::NPackager::NTemp {

    TMP4VodTestWorker::TMP4VodTestWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ true, "packager_test_mp4_vod:")
    {
    }

    // static
    void TMP4VodTestWorker::CheckConfig(const TLocationConfig& config) {
        TSourceMp4File::TConfig(config).Check();
        TSourceUnion::TConfig(config).Check();
        TDispatcher::TConfig(config).Check();
    }

    void TMP4VodTestWorker::Work() {
        if (0)
            TestUnion();

        const TSourceMp4File::TConfig mp4SourceConfig(Config);

#if 1
        // // const auto sourceFuture = TSourceMp4File::MakeTSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r/sample.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r_imp/sample.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r_imp/source.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r_imp/video_1_b7392a12a8f8981ddf65b9377a2f0d9a.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/video_1_b7392a12a8f8981ddf65b9377a2f0d9a.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/audio_4_822ba9b8db9d8820099c10de5aa938b6.mp4", "");

        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/7201989777523249220_169_144p-1626883655000.mp4", ""); // stub !!
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/7201989777523249220_169_720p-1626872990000.mp4", ""); // hd chunk
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/7201989777523249220_169_144p-1626883455000.mp4", ""); // low chunk
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/test1v2a.mp4", ""); // low chunk
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/test1v2a.mp4", "", /*cropInterval = */ {}, /*offset = */ {}, /*getWholeFile = */ true); // low chunk

        // const auto sourceFuture = TSourceWebvttFile::Make(*this, "/mp4prox_aio/subtitles.vtt", "");
        // const auto sourceFuture = TSourceWebvttFile::Make(*this, "/mp4prox_aio/subtitles.vtt", "", TIntervalP{Ms2P(5000), Ms2P(20000)}, Ms2P(5000));

        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/dlbv/video_dv_hevc_3840x1920_2500_11600_2160p.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/audio_0_ec3_6ch_640_rus.mp4", "");

        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_r_imp_echo/source.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/err_prox/source.mp4", "");  // TODO: investigate

        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/sample.mp4", "");
        // // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox/sample.mp4", "");
        // // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/sample.mp4", "");  // TODO: research, why in this case works bad sometimmes (https://st.yandex-team.ru/STRM-3211)

        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/localproxy/video.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/s3video.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/video.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/source_new1.mp4", "");
        const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/source.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/ad.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/source_4.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/stub.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/source_gap_left_0.66.mp4", "");
        // const auto sourceFuture = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/source_gap_left_0.09.mp4", "");
        // const auto sourceFuture = TSourceUnion::Make(
        //     *this,
        //     Config,
        //     TSourceMp4File::Make(
        //         *this,
        //         mp4SourceConfig,
        //         // "/"
        //         "/mp4prox_no_cache/"
        //         // "/mp4prox_aio/"

        //         "/mp4prox_aio/vp9_source_new1.mp4",
        //         // "/mp4prox_aio/source_new1.mp4",
        //         // "source_4.mp4",
        //         // "ad.mp4",
        //         // "source.mp4",
        //         // "stub.mp4",
        //         // "source_gap_left_0.66.mp4",
        //         // "source_video_gap_left.mp4",
        //         // "source_video_gap_right.mp4",
        //         // "source_video_gaps.mp4",
        //         // "notexist",
        //         ""),
        //     [this]() {
        //         return TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/stub.mp4", "");
        //     });

#else
        TSourceFuture sourceFuture;
        {
            const auto av = TSourceMp4File::Make(*this, mp4SourceConfig, "/mp4prox_aio/source.mp4", "", TIntervalP{Ms2P(0), Ms2P(10000)});
            const auto s = TSourceWebvttFile::Make(*this, "/mp4prox_aio/subtitles.vtt", "", TIntervalP{Ms2P(0), Ms2P(10000)});
            sourceFuture = TSourceTracksSelect::Make(
                *this,
                {
                    TSourceTracksSelect::TTrackFromSourceFuture{TSourceTracksSelect::ETrackType::Video, 0, av},
                    TSourceTracksSelect::TTrackFromSourceFuture{TSourceTracksSelect::ETrackType::Audio, 0, av},
                    TSourceTracksSelect::TTrackFromSourceFuture{TSourceTracksSelect::ETrackType::Subtitle, 0, s},
                });
            sourceFuture = ConvertSubtitlesRawToTTML(*this, sourceFuture);
        }
#endif

        const auto muxerFuture = TMuxerMP4::Make(*this);
        // const auto muxerFuture = TMuxerWebvtt::Make(*this);
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

                // source->GetMedia(TIntervalP{4000, 6000}, 0);
                // source->GetMedia(TIntervalP{5000, 8000}, 0);

                if (0) {
                    const TIntervalP sourceFullInterval = source->FullInterval();
                    Clog << "source full interval: " << sourceFullInterval.Begin << " " << sourceFullInterval.End << Endl;
                }

                TIntervalP si = source->FullInterval();
                auto metaPromise = TRepPromise<TMetaData>::Make();

#if 1
                metaPromise.PutData(TMetaData{
                    .Interval = si,
                    .Data = TMetaData::TSourceData(),
                });
#else
                const Ti64TimeP ms5k = Ms2P(5000);
                si.End = si.End / ms5k * ms5k;

                // just for test
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
                        d.Data = TMetaData::TAdData{.BlockBegin = Ti64TimeP(0)};
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

                SetContentType(muxer->GetContentType(source->GetTracksInfo()));

                // const auto dispatcher = GetPoolUtil<TDispatcher>().New(*this, TDispatcher::TConfig(Config), meta, *source);
                // // muxer->SetMediaData(dispatcher->GetMediaData());
                // muxer->SetMediaData(TFragmentCutter::Cut(dispatcher->GetMediaData(), Ms2P(500), *this));
                // // muxer->SetMediaData(TFragmentCutter::Cut(dispatcher->GetMediaData(), 500000, *this));

                // muxer->SetMediaData(source->GetMedia(TIntervalP{Ms2P(0), Ms2P(5000 * 12)}, Ms2P(0)));
                // muxer->SetMediaData(source->GetMedia(TIntervalP{Ms2P(15000), Ms2P(21000)}, Ms2P(5000)));
                // muxer->SetMediaData(source->GetMedia(TIntervalP{Ms2P(15000), Ms2P(21000)}, Ms2P(0)));
                muxer->SetMediaData(source->GetMedia());

                // muxer->SetMediaData(source->GetMedia(TInterval{0, 4004}, 0));
                // muxer->SetMediaData(source->GetMedia(TInterval{5000, 10000}, -5000));
                sender->SetData(muxer->GetData());
            }));
    }

}
