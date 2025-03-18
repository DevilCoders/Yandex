#include <nginx/modules/strm_packager/src/workers/kaltura_imitation_worker.h>

#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/common/convert_subtitles_raw_to_ttml.h>
#include <nginx/modules/strm_packager/src/common/drm_info_util.h>
#include <nginx/modules/strm_packager/src/common/fragment_cutter.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>
#include <nginx/modules/strm_packager/src/common/muxer_mp4.h>
#include <nginx/modules/strm_packager/src/common/muxer_mpegts.h>
#include <nginx/modules/strm_packager/src/common/muxer_webvtt.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>
#include <nginx/modules/strm_packager/src/common/sender.h>
#include <nginx/modules/strm_packager/src/common/source.h>
#include <nginx/modules/strm_packager/src/common/source_concat.h>
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>
#include <nginx/modules/strm_packager/src/common/source_tracks_select.h>
#include <nginx/modules/strm_packager/src/common/source_webvtt_file.h>
#include <nginx/modules/strm_packager/src/content/live_uri.h>

#include <library/cpp/json/json_reader.h>

#include <util/stream/mem.h>
#include <util/string/split.h>

namespace NStrm::NPackager {
    TKalturaImitationWorker::TKalturaImitationWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ false, "packager_kaltura_imitation_worker:")
    {
    }

    // static
    void TKalturaImitationWorker::CheckConfig(const TLocationConfig& config) {
        TSourceMp4File::TConfig(config).Check();
        Y_ENSURE(config.PlaylistJsonUri.Defined());
        Y_ENSURE(config.PlaylistJsonArgs.Defined());
        Y_ENSURE(config.ContentLocation.Defined());
    }

    TKalturaImitationWorker::TChunkInfo TKalturaImitationWorker::MakeChunkInfo() const {
        const TStringBuf uri = Config.URI.Defined() ? GetComplexValue(Config.URI.GetRef()) : GetUri();

        const TVector<TStringBuf> parts = StringSplitter(uri).Split('/').SkipEmpty();
        Y_ENSURE_EX(parts.size() > 0, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad uri : parts empty");

        const TStringBuf chunk = parts.back();
        const TChunkInfo chunkInfo = ParseCommonChunkInfo(chunk);

        Y_ENSURE_EX(chunkInfo.Subtitle.Defined() || chunkInfo.Video.Defined() || chunkInfo.Audio.Defined(), THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad chunk '" << chunk << "'");
        Y_ENSURE_EX(chunkInfo.PartIndex.Empty(), THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad chunk '" << chunk << "'");

        return chunkInfo;
    }

    void TKalturaImitationWorker::Work() {
        DisableChunkedTransferEncoding();

        // make info first to do early 404 errors
        const TChunkInfo chunkInfo = MakeChunkInfo();

        // go in playlist-json
        const NThreading::TFuture<TBuffer> kalturaJson = CreateSubrequest(
            TSubrequestParams{
                .Uri = (TString)GetComplexValue(*Config.PlaylistJsonUri),
                .Args = (TString)GetComplexValue(*Config.PlaylistJsonArgs),
            },
            NGX_HTTP_OK);

        TSourceFuture sourceFuture = kalturaJson.Apply([this, chunkInfo](const NThreading::TFuture<TBuffer>& future) {
            future.TryRethrow();
            const TBuffer& buf = future.GetValue();
            TMemoryInput bufInput(buf.Data(), buf.Size());

            const NJson::TJsonValue jval = NJson::ReadJsonTree(&bufInput, /*throwOnError = */ true);
            Y_ENSURE(jval.IsMap());
            const NJson::TJsonValue::TMapType& jmap = jval.GetMapSafe();

            const bool discontinuity = jmap.at("discontinuity").GetBooleanSafe();
            Y_ENSURE(!discontinuity);
            // dont look at clipTimes since discontinuity == false

            const i64 segmentBaseTime = jmap.at("segmentBaseTime").GetIntegerSafe();
            Y_ENSURE(segmentBaseTime == 0); // no need to suport segmentBaseTime

            const Ti64TimeP firstClipTime = Ms2P(jmap.at("firstClipTime").GetIntegerSafe());
            Y_ENSURE(firstClipTime.Value >= 0);

            const Ti64TimeP segmentDuration = Ms2P(jmap.at("segmentDuration").GetIntegerSafe());
            Y_ENSURE(segmentDuration.Value > 0);

            const auto& sequences = jmap.at("sequences").GetArraySafe();
            Y_ENSURE(sequences.size() == 1); // enought for now (for subtitles)

            const auto& clips = sequences[0].GetMapSafe().at("clips").GetArraySafe();
            const auto durations = jmap.at("durations").GetArraySafe();
            Y_ENSURE(durations.size() == clips.size());

            TVector<std::pair<TIntervalP, TString>> paths(clips.size());

            Ti64TimeP start = firstClipTime;
            for (size_t i = 0; i < clips.size(); ++i) {
                const Ti64TimeP duration = Ms2P(durations[i].GetIntegerSafe());

                auto& p = paths[i];

                p.first.Begin = start;
                p.first.End = start + duration;

                start += duration;

                const auto& clip = clips[i].GetMapSafe();
                if (clip.at("type").GetStringSafe() == "source") {
                    p.second = clip.at("path").GetStringSafe();
                } else {
                    p.second = clip.at("source").GetMapSafe().at("path").GetStringSafe();
                }
            }

            const TIntervalP interval = {
                .Begin = (chunkInfo.ChunkIndex - 1) * segmentDuration,
                .End = chunkInfo.ChunkIndex * segmentDuration,
            };

            const TStringBuf contentLocation = GetComplexValue(*Config.ContentLocation);
            const TSourceMp4File::TConfig mp4Config(Config);

            TVector<TSourceFuture> toConcat;

            for (const auto& p : paths) {
                const TIntervalP intersection = chunkInfo.IsInit
                                                    ? p.first
                                                    : TIntervalP{
                                                          .Begin = Max(p.first.Begin, interval.Begin),
                                                          .End = Min(p.first.End, interval.End),
                                                      };

                if (intersection.End <= intersection.Begin) {
                    continue;
                }

                const Ti64TimeP offset = p.first.Begin;
                const TIntervalP crop = {
                    .Begin = intersection.Begin - offset,
                    .End = intersection.End - offset,
                };

                TSourceFuture src;
                if (p.second.EndsWith(".vtt")) {
                    src = TSourceWebvttFile::Make(*this, contentLocation + p.second, TString(), crop, offset);
                } else {
                    src = TSourceMp4File::Make(*this, mp4Config, contentLocation + p.second, TString(), crop, offset);
                }

                toConcat.push_back(CommonLiveSelectTracks(*this, chunkInfo, src));

                if (chunkInfo.IsInit) {
                    break;
                }
            }

            Y_ENSURE(toConcat.size() > 0);

            return TSourceConcat::Make(*this, toConcat);
        });

        if (chunkInfo.Container == EContainer::MP4) {
            sourceFuture = ConvertSubtitlesRawToTTML(*this, sourceFuture);
        }

        TMuxerFuture muxerFuture;
        switch (chunkInfo.Container) {
            case EContainer::WEBVTT:
                muxerFuture = TMuxerWebvtt::Make(*this);
                break;
            case EContainer::MP4:
                muxerFuture = TMuxerMP4::Make(*this, NThreading::MakeFuture<TMaybe<TDrmInfo>>(), /*contentLengthPromise = */ false, /*addServerTimeUuidBoxes = */ false);
                break;
            case EContainer::TS:
                Y_ENSURE(chunkInfo.ChunkIndex > 0);
                muxerFuture = TMuxerMpegTs::Make(*this, chunkInfo.ChunkIndex, NThreading::MakeFuture<TMaybe<TDrmInfo>>());
                break;

            default:
                Y_ENSURE(false, "failed to create muxer: container '" << chunkInfo.Container << "' unsupported");
        }

        const auto senderFuture = TSender::Make(*this);

        const auto resultFuture = PackagerWaitExceptionOrAll(TVector{
            sourceFuture.IgnoreResult(),
            muxerFuture.IgnoreResult(),
            senderFuture.IgnoreResult(),
        });

        resultFuture.Subscribe(MakeFatalOnException(
            [this, sourceFuture, muxerFuture, senderFuture, init = chunkInfo.IsInit](const NThreading::TFuture<void>& resultFuture) {
                resultFuture.TryRethrow();

                const auto source = sourceFuture.GetValue();
                const auto muxer = muxerFuture.GetValue();
                const auto sender = senderFuture.GetValue();

                SetContentType(muxer->GetContentType(source->GetTracksInfo()));

                if (init) {
                    SendData({muxer->MakeInitialSegment(source->GetTracksInfo()), false});
                    Finish();
                    return;
                }

                const TIntervalP interval = source->FullInterval();

                TRepFuture<TMediaData> mediaData = source->GetMedia();

                mediaData = TFragmentCutter::Cut(mediaData, interval.End - interval.Begin, *this);

                muxer->SetMediaData(mediaData);

                sender->SetData(muxer->GetData());
            }));
    }

}
