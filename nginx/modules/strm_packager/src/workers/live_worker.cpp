#include <nginx/modules/strm_packager/src/workers/live_worker.h>

#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/common/drm_info_util.h>
#include <nginx/modules/strm_packager/src/common/fragment_cutter.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>
#include <nginx/modules/strm_packager/src/common/muxer_mp4.h>
#include <nginx/modules/strm_packager/src/common/muxer_mpegts.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>
#include <nginx/modules/strm_packager/src/common/sender.h>
#include <nginx/modules/strm_packager/src/common/source.h>
#include <nginx/modules/strm_packager/src/common/source_concat.h>
#include <nginx/modules/strm_packager/src/common/source_live.h>
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>
#include <nginx/modules/strm_packager/src/common/source_tracks_select.h>
#include <nginx/modules/strm_packager/src/common/source_union.h>

#include <util/string/split.h>

namespace NStrm::NPackager {
    TLiveWorker::TLiveWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ false, "packager_live_worker:")
    {
    }

    // static
    void TLiveWorker::CheckConfig(const TLocationConfig& config) {
        TSourceUnion::TConfig(config).Check();
        TSourceLive::TConfig(config).Check();
        TSourceMp4File::TConfig(config).Check();
        Y_ENSURE(config.LiveManager);
        Y_ENSURE(config.ContentLocation.Defined());
        Y_ENSURE(config.TranscodersLiveLocationCacheFollow.Defined());
        Y_ENSURE(config.TranscodersLiveLocationCacheLock.Defined());
        Y_ENSURE(config.LiveVideoTrackName.Defined());
        Y_ENSURE(config.LiveAudioTrackName.Defined());
        Y_ENSURE(config.LiveCmafFlag.Defined());
    }

    TLiveWorker::TChunkInfo TLiveWorker::MakeChunkInfo() const {
        const TStringBuf uri = Config.URI.Defined() ? GetComplexValue(Config.URI.GetRef()) : GetUri();

        const TVector<TStringBuf> parts = StringSplitter(uri).Split('/').SkipEmpty();

        Y_ENSURE_EX(parts.size() == 3, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad uri : parts.size(): " << parts.size() << " != 3");
        Y_ENSURE_EX(parts[0] == "live", THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad uri : parts[0]: " << parts[0] << " != live");
        const TStringBuf chunk = parts.back();

        TChunkInfo chunkInfo;
        static_cast<TCommonChunkInfo&>(chunkInfo) = ParseCommonChunkInfo(chunk);

        Y_ENSURE_EX(chunkInfo.Subtitle.Empty(), THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad chunk '" << chunk << "'");
        Y_ENSURE_EX(chunkInfo.Container != EContainer::WEBVTT, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "bad chunk '" << chunk << "'");

        chunkInfo.Uuid = parts[1];

        chunkInfo.VideoTrackName = GetComplexValue(*Config.LiveVideoTrackName);
        chunkInfo.AudioTrackName = GetComplexValue(*Config.LiveAudioTrackName);

        // Temporary, TODO: enable consistency checks below
        if (chunkInfo.Video.Defined()) {
            Y_ENSURE_EX(!chunkInfo.VideoTrackName.Empty(), THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "Inconsistent video request");
        }
        // Y_ENSURE_EX(chunkInfo.VideoTrackName.Empty() == chunkInfo.Video.Empty(), THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "Inconsistent video request");
        // Y_ENSURE_EX(chunkInfo.AudioTrackName.Empty() == chunkInfo.Audio.Empty(), THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "Inconsistent audio request");

        if (chunkInfo.VideoTrackRequested()) {
            Y_ENSURE_EX(chunkInfo.Video == 1u, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "In case video track is requested, request v1");
        }
        if (chunkInfo.AudioTrackRequestedNew()) {
            Y_ENSURE_EX(chunkInfo.Audio == 1u, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "In case audio track name is specified, request a1");
        }
        const bool anythingRequested = chunkInfo.VideoTrackRequested() || chunkInfo.AudioTrackRequestedNew() || chunkInfo.AudioTrackRequestedOld();
        Y_ENSURE_EX(anythingRequested, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "No track is properly requested");

        chunkInfo.Cmaf = GetComplexValue(*Config.LiveCmafFlag) == "1";

        return chunkInfo;
    }

    void TLiveWorker::Work() {
        const bool lowLatencyMode = GetArg<bool>("lowlatency", false).GetOrElse(false);
        GetMetrics().LiveIsLowLatencyMode = lowLatencyMode ? 1 : 0;

        TChunkInfo chunkInfo = MakeChunkInfo();
        GetMetrics().IsCmaf = chunkInfo.Cmaf ? 1 : 0;

        TLiveManager::TStreamData* const streamData = Config.LiveManager->Find(chunkInfo.Uuid);
        Y_ENSURE_EX(streamData, THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "uuid " << chunkInfo.Uuid << " not known");

        const NLiveDataProto::TStream& streamInfo = streamData->GetStream();

        if (chunkInfo.IsInit) {
            chunkInfo.ChunkIndex = streamInfo.GetFirstChunkIndex();
        }

        Y_ENSURE_EX(chunkInfo.ChunkIndex >= streamInfo.GetFirstChunkIndex(),
                    THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "chunk index " << chunkInfo.ChunkIndex << " less than first chunk index " << streamInfo.GetFirstChunkIndex());

        Y_ENSURE_EX(Config.LiveFutureChunkLimit.Empty() || chunkInfo.ChunkIndex <= streamInfo.GetLastChunkIndex() + *Config.LiveFutureChunkLimit + (lowLatencyMode ? 1 : 0),
                    THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "chunk index too far in the future: " << chunkInfo.ChunkIndex << " last: " << streamInfo.GetLastChunkIndex());

        const TCommonDrmInfo drmInfo = GetCommonDrmInfo(*this, chunkInfo.ChunkIndex);

        const bool serverTimeUuidBox =
            lowLatencyMode &&
            chunkInfo.Cmaf &&
            chunkInfo.PartIndex.Empty() &&
            chunkInfo.Container == EContainer::MP4 &&
            !chunkInfo.IsInit &&
            chunkInfo.ChunkIndex > streamInfo.GetLastChunkIndex();
        GetMetrics().LiveWriteServerTimeUuidBox = serverTimeUuidBox ? 1 : 0;

        if (!serverTimeUuidBox) {
            DisableChunkedTransferEncoding();
        }

        if (serverTimeUuidBox) {
            NMP4Muxer::TBufferWriter writer;
            TServerTimeUuidBox(/*with header = */ true).Write(writer);
            SendData({.Blob = HangDataInPool(std::move(writer.Buffer())), .Flush = true});
        }

        auto sourcePromise = NewSourcePromise();

        auto fulfillSourcePromise = [this, sourcePromise, chunkInfo, lowLatencyMode](const NLiveDataProto::TStream& streamInfo) {
            Y_ENSURE(streamInfo.GetChunkDuration() == streamInfo.GetSegmentDuration());
            const Ti64TimeP chunkDuration = Ms2P(streamInfo.GetChunkDuration());
            const Ti64TimeP chunkFragmentDuration = Ms2P(chunkInfo.PartIndex.Defined() ? streamInfo.GetHLSPartDuration() : streamInfo.GetChunkFragmentDuration());

            ChunkInterval = TSourceLive::MakeInterval(chunkInfo.ChunkIndex, {}, chunkDuration, chunkFragmentDuration);
            CutInterval = TSourceLive::MakeInterval(chunkInfo.ChunkIndex, chunkInfo.PartIndex, chunkDuration, chunkFragmentDuration);

            auto makeTrackSourceFuture = [&](const TString& trackName) {
                TVector<TSourceUnion::TSourceMaker> sourceMakers;
                if (lowLatencyMode && chunkInfo.ChunkIndex > streamInfo.GetLastChunkIndex()) {
                    sourceMakers.push_back(CreateTranscoderSourceLowLatency(streamInfo, chunkInfo, trackName, chunkDuration, chunkFragmentDuration));
                }

                sourceMakers.push_back(CreateCompletedChunkSource(streamInfo, chunkInfo, trackName));

                return TSourceUnion::Make(
                    *this,
                    TSourceUnion::TConfig(Config),
                    sourceMakers);
            };

            TVector<TSourceTracksSelect::TTrackFromSourceFuture> tracks;

            TSourceFuture videoSource;
            if (chunkInfo.VideoTrackRequested() || chunkInfo.AudioTrackRequestedOld()) {
                videoSource = makeTrackSourceFuture(chunkInfo.VideoTrackName);
            }

            if (chunkInfo.VideoTrackRequested()) {
                tracks.push_back({TSourceTracksSelect::ETrackType::Video, *chunkInfo.Video - 1, videoSource});
            }

            if (chunkInfo.AudioTrackRequestedNew()) {
                TSourceFuture audioSource = makeTrackSourceFuture(chunkInfo.AudioTrackName);
                tracks.push_back({TSourceTracksSelect::ETrackType::Audio, *chunkInfo.Audio - 1, audioSource});
            }

            if (chunkInfo.AudioTrackRequestedOld()) { // TODO: Remove this
                tracks.push_back({TSourceTracksSelect::ETrackType::Audio, *chunkInfo.Audio - 1, videoSource});
            }

            Y_ENSURE(chunkInfo.Subtitle.Empty()); // TODO: Support subtitles

            TransferFutureToPromise(TSourceTracksSelect::Make(*this, tracks), sourcePromise);
        };

        if (lowLatencyMode && chunkInfo.ChunkIndex > streamInfo.GetLastChunkIndex() + 1) {
            Config.LiveManager->Subscribe(
                *this,
                *streamData,
                chunkInfo.ChunkIndex - 1,
                std::move(fulfillSourcePromise));
        } else {
            fulfillSourcePromise(streamInfo);
        }

        const auto sourceFuture = sourcePromise.GetFuture();
        TMuxerFuture muxerFuture;
        switch (chunkInfo.Container) {
            case EContainer::MP4:
                muxerFuture = TMuxerMP4::Make(*this, drmInfo.Info4Muxer, /*contentLengthPromise = */ false, /*addServerTimeUuidBoxes = */ serverTimeUuidBox);
                break;
            case EContainer::TS:
                Y_ENSURE(chunkInfo.ChunkIndex > 0);
                muxerFuture = TMuxerMpegTs::Make(*this, chunkInfo.ChunkIndex, drmInfo.Info4Muxer);
                break;
            default:
                Y_ENSURE(false, "failed to create muxer: container '" << chunkInfo.Container << "' unsupported");
        }

        const auto senderFuture = TSender::Make(*this, drmInfo.Info4Sender);

        const auto resultFuture = PackagerWaitExceptionOrAll(TVector{
            sourceFuture.IgnoreResult(),
            muxerFuture.IgnoreResult(),
            senderFuture.IgnoreResult(),
        });

        resultFuture.Subscribe(MakeFatalOnException(
            [this, sourceFuture, muxerFuture, senderFuture, init = chunkInfo.IsInit, cmaf = chunkInfo.Cmaf, partIndex = chunkInfo.PartIndex](const NThreading::TFuture<void>& resultFuture) {
                resultFuture.TryRethrow();

                const auto source = sourceFuture.GetValue();
                const auto muxer = muxerFuture.GetValue();
                const auto sender = senderFuture.GetValue();

                SetContentType(muxer->GetContentType(source->GetTracksInfo()));

                if (init) {
                    DisableChunkedTransferEncoding();
                    SendData({muxer->MakeInitialSegment(source->GetTracksInfo()), false});
                    Finish();
                    return;
                }

                TRepFuture<TMediaData> mediaData = source->GetMedia();

                if (cmaf && partIndex.Empty()) {
                    // TODO: configure cmaf fragment size
                    mediaData = TFragmentCutter::Cut(mediaData, Ms2P(500), *this);
                } else {
                    DisableChunkedTransferEncoding();
                    mediaData = TFragmentCutter::Cut(mediaData, CutInterval->End - CutInterval->Begin, *this);
                }

                muxer->SetMediaData(mediaData);

                sender->SetData(muxer->GetData());
            }));
    }

    TSourceUnion::TSourceMaker TLiveWorker::CreateCompletedChunkSource(const NLiveDataProto::TStream& streamInfo, const TChunkInfo& chunkInfo, const TString& trackName) {
        // s3 url example:
        // http://s3.mds.yandex.net/vh-zen-converted/vod-content/7713367299236893445/live/kaltura/chunks/77133672992368934450_169_144p-1616579005000.mp4
        //   for bucket = "vh-zen-converted", uuid = "7713367299236893445", quality = "169_144p"

        // Config.ContentLocation - same as vod_remote_upstream_location like "/kalproxy/video_chunks/live/" and shard
        //   i.e. "/kalproxy/video_chunks/live/$shard/"
        TStringBuilder uri;
        uri << GetComplexValue(*Config.ContentLocation) << streamInfo.GetS3Bucket() << "/vod-content/" << chunkInfo.Uuid << "/live/kaltura/chunks/" << chunkInfo.Uuid << "0_" << trackName << "-" << ChunkInterval->End.ConvertToTimescale(1000) << ".mp4";

        auto maker = [this, chunkInfo, uri = (TString)uri, getWholeFile = chunkInfo.PartIndex.Empty()]() -> TSourceFuture {
            return TSourceMp4File::Make(
                *this,
                TSourceMp4File::TConfig(Config),
                uri,
                /*args = */ "",
                TIntervalP{CutInterval->Begin - ChunkInterval->Begin, CutInterval->End - ChunkInterval->Begin},
                ChunkInterval->Begin,
                getWholeFile);
        };

        if (chunkInfo.ChunkIndex <= streamInfo.GetLastChunkIndex()) {
            return maker;
        }

        return [maker, this, uuid = chunkInfo.Uuid, chunkIndex = chunkInfo.ChunkIndex]() -> TSourceFuture {
            TLiveManager::TStreamData* const streamData = Config.LiveManager->Find(uuid);
            if (!streamData) {
                return NThreading::MakeErrorFuture<ISource*>(std::make_exception_ptr(THttpError(NGX_HTTP_NOT_FOUND, TLOG_WARNING) << "CreateCompletedChunkSource: uuid " << uuid << " not known"));
            }

            TSourcePromise promise = NewSourcePromise();

            Config.LiveManager->Subscribe(
                *this,
                *streamData,
                chunkIndex,
                [this, maker, promise](const NLiveDataProto::TStream& info) mutable {
                    (void)info;
                    maker().Subscribe(MakeFatalOnException([promise](const TSourceFuture& source) mutable {
                        try {
                            source.TryRethrow();
                        } catch (const TFatalExceptionContainer&) {
                            throw;
                        } catch (...) {
                            promise.SetException(std::current_exception());
                        }
                        promise.SetValue(source.GetValue());
                    }));
                });

            return promise.GetFuture();
        };
    }

    TSourceUnion::TSourceMaker TLiveWorker::CreateTranscoderSourceLowLatency(
        const NLiveDataProto::TStream& streamInfo,
        const TChunkInfo& chunkInfo,
        const TString& trackName,
        const Ti64TimeP chunkDuration,
        const Ti64TimeP chunkFragmentDuration) {
        Y_ENSURE(streamInfo.ChunkRangesSize() > 0);

        const auto& range = *streamInfo.GetChunkRanges().rbegin();

        // low latency source url example
        // /packproxy/live/(?<bucket>[\w\-]+)/transcoder/(?<trns_host>[^/]+)/low_latency/(?<trns_port>\d+)/(?<uuid>\w+)/(?<target_tracks>.+)
        TStringBuilder uri;
        if (chunkInfo.PartIndex.Defined() && Config.TranscodersLiveLocationCacheLock.Defined()) { // TODO: dont check TranscodersLiveLocationCacheLock.Defined() after configs update
            uri << *Config.TranscodersLiveLocationCacheLock;
        } else {
            uri << *Config.TranscodersLiveLocationCacheFollow;
        }
        uri << streamInfo.GetS3Bucket() << "/transcoder/" << range.GetTranscoder().GetHost() << "/low_latency/" << range.GetTranscoder().GetHttpPort() << "/" << chunkInfo.Uuid;

        return [this,
                chunkInfo,
                trackName,
                uri = (TString)uri,
                chunkDuration,
                chunkFragmentDuration]() -> TSourceFuture {
            return TSourceLive::Make(
                *this,
                TSourceLive::TConfig(Config),
                uri,
                trackName,
                chunkInfo.ChunkIndex,
                chunkInfo.PartIndex,
                chunkDuration,
                chunkFragmentDuration);
        };
    }

}
