#include <nginx/modules/strm_packager/src/workers/vod_worker.h>

#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/common/fragment_cutter.h>
#include <nginx/modules/strm_packager/src/common/muxer_mp4.h>
#include <nginx/modules/strm_packager/src/common/muxer_mpegts.h>
#include <nginx/modules/strm_packager/src/common/sender.h>
#include <nginx/modules/strm_packager/src/common/source_concat.h>
#include <nginx/modules/strm_packager/src/common/source_tracks_select.h>
#include <nginx/modules/strm_packager/src/content/description.h>
#include <nginx/modules/strm_packager/src/content/vod_description.h>
#include <nginx/modules/strm_packager/src/content/vod_prerolls.h>
#include <nginx/modules/strm_packager/src/content/vod_uri.h>

namespace NStrm::NPackager {
    TVodWorker::TVodWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ true, "packager_vod_worker:")
    {
    }

    // static
    void TVodWorker::CheckConfig(const TLocationConfig& config) {
        TSourceMp4File::TConfig(config).Check();
        Y_ENSURE(config.MetaLocation.Defined());
        Y_ENSURE(config.ContentLocation.Defined());

        CheckPrerollsConfig(config);
    }

    void TVodWorker::Work() {
        const bool allowEarlyHeaders = GetArg<bool>("early_headers", false).GetOrElse(false);
        if (!allowEarlyHeaders) {
            DisableChunkedTransferEncoding();
        }

        const auto parsedUri = GetPoolUtil<TVodUri>().New(
            Config.URI.Defined()
                ? GetComplexValue(Config.URI.GetRef())
                : GetUri());

        const auto getDescriptionFuture = [this](const TString& descriptionUri) -> NThreading::TFuture<const NDescription::TDescription*> {
            const auto descriptionGetter = [this, descriptionUri]() {
                return CreateSubrequest(
                    TSubrequestParams{
                        .Uri = descriptionUri,
                        .Args = "",
                    },
                    NGX_HTTP_OK);
            };

            const auto descriptionLoader = [this](const void* buffer, size_t bufferSize) {
                const auto data = GetPoolUtil<TBuffer>().New((const char*)buffer, bufferSize);
                return NFb::GetTDescription(data->Data());
            };

            const auto descriptionSaver = [this](const TBuffer& buffer) {
                return ParseVodDescription(*this, buffer);
            };

            if (!Config.DescriptionShmCacheZone.Defined()) {
                return descriptionGetter().Apply(
                    [this, descriptionSaver](const NThreading::TFuture<TBuffer>& future) mutable {
                        const auto buffer = descriptionSaver(future.GetValue());
                        return NFb::GetTDescription(HangDataInPool(std::move(buffer)).begin());
                    });
            } else {
                return Config.DescriptionShmCacheZone->Data().Get<const NDescription::TDescription*, TBuffer, TBuffer>(
                    *this,
                    descriptionUri,
                    descriptionGetter,
                    descriptionLoader,
                    descriptionSaver);
            }
        };

        const TVector<TString> prerolls = parsedUri->GetChunkInfo().IsInitSegment ? TVector<TString>()
                                                                                  : GetPrerolls(*this);

        TVector<NThreading::TFuture<const NDescription::TDescription*>> descriptions; // all prerols descriptions + main content description
        descriptions.reserve(prerolls.size() + 1);
        for (const TString& preroll : prerolls) {
            descriptions.push_back(getDescriptionFuture(GetDescriptionUri(preroll)));
        }

        const TString descUri = GetDescriptionUri(parsedUri->GetDescriptionUri());
        LogInfo() << "url parsed: desc_uri=" << descUri;
        descriptions.push_back(getDescriptionFuture(descUri));

        const TCommonDrmInfo drmInfo = GetCommonDrmInfo(*this, parsedUri->GetChunkInfo().Number);

        const TSourceFuture sourceFuture = PackagerWaitExceptionOrAll(descriptions).Apply( // wait all descriptions
            [this, parsedUri, drmInfo, descriptions = std::move(descriptions), prerolls = std::move(prerolls)](const NThreading::TFuture<void>& future) mutable {
                future.TryRethrow();

                NDescription::TDescription const* const description = descriptions.back().GetValue(); // main content description
                Y_ENSURE(description);

                const TVodUri::TTrackInfo& trackInfo = parsedUri->GetTrackInfo();
                const TVodUri::TChunkInfo& chunkInfo = parsedUri->GetChunkInfo();

                if (description->WithDRM()) {
                    Y_ENSURE_EX(drmInfo.Enabled || drmInfo.AllowDrmContentUnencrypted, THttpError(NGX_HTTP_FORBIDDEN, TLOG_ERR) << "drm flags inconsistence");
                } else if (drmInfo.Enabled) {
                    Y_ENSURE_EX(drmInfo.WholeSegmentAes128, THttpError(NGX_HTTP_FORBIDDEN, TLOG_ERR) << "only aes128 drm available for not drm content");
                }

                LogInfo() << "get_sources:"
                          << " video_track_id=" << trackInfo.VideoTrackId
                          << " audio_track_id=" << trackInfo.AudioTrackId;

                const TSourceMp4File::TConfig mp4SourceConfig(Config);

                const Ti64TimeMs chunkDuration = GetChunkDuration(description);

                for (size_t si = 0; si < prerolls.size(); ++si) {
                    Y_ENSURE(GetChunkDuration(descriptions[si].GetValue()) == chunkDuration);
                }

                const TIntervalMs interval = chunkInfo.IsInitSegment
                                                 ? TIntervalMs()
                                                 : parsedUri->GetInterval(chunkDuration);

                SegmentInterval = TIntervalP(interval);

                TVector<TSourceTracksSelect::TTrackFromSourceFuture> sources;

                for (int tk = 0; tk < 2; ++tk) {
                    const bool video = (tk == 0);

                    const auto trackInfoTrackId = video ? &TVodUri::TTrackInfo::VideoTrackId
                                                        : &TVodUri::TTrackInfo::AudioTrackId;

                    const auto sTrackSelectType = video ? TSourceTracksSelect::ETrackType::Video
                                                        : TSourceTracksSelect::ETrackType::Audio;

                    const auto descGetTrackFunc = video ? NDescription::GetVideoTrack
                                                        : NDescription::GetAudioTrack;

                    const auto descGetTracksSet = video ? &NDescription::TDescription::VideoSets
                                                        : &NDescription::TDescription::AudioSets;

                    if ((trackInfo.*trackInfoTrackId).Empty()) {
                        continue;
                    }

                    TVector<NDescription::TSourceInfo> sourceInfos;
                    Ti64TimeMs accDuration(0);

                    for (size_t si = 0; si < descriptions.size(); ++si) {
                        const bool main = (si + 1 == descriptions.size());

                        const int trackId = main
                                                ? *(trackInfo.*trackInfoTrackId)
                                                : SelectPrerollTrackId(
                                                      descGetTracksSet,
                                                      *descriptions[si].GetValue(),
                                                      *descriptions.back().GetValue(),
                                                      *(trackInfo.*trackInfoTrackId));

                        const auto track = descGetTrackFunc(descriptions[si].GetValue(), trackId);

                        if (chunkInfo.IsInitSegment) {
                            AddInitSourceInfo(track->Segments(), sourceInfos);
                            break;
                        }

                        Y_ENSURE(accDuration.Value % chunkDuration.Value == 0);

                        NDescription::AddSourceInfos(track->Segments(), TIntervalMs{.Begin = interval.Begin - accDuration, .End = interval.End - accDuration}, accDuration, sourceInfos);

                        accDuration += Ti64TimeMs(track->Duration());
                    }

                    Y_ENSURE_EX(!sourceInfos.empty(), THttpError(404, TLOG_WARNING) << "no sources, interval: " << interval.Begin << " - " << interval.End << " accDuration: " << accDuration);

                    // TODO: support multiple tracks (more than 0 track index)
                    sources.emplace_back(TSourceTracksSelect::TTrackFromSourceFuture{
                        sTrackSelectType,
                        0, // = TrackIndex
                        TSourceConcat::Make(*this, MakeSources(sourceInfos, mp4SourceConfig))});
                }

                Y_ENSURE_EX(!sources.empty(), THttpError(404, TLOG_WARNING) << "didn't find any video or audio sources");

                return TSourceTracksSelect::Make(*this, sources);
            });

        const TVodUri::TChunkInfo& chunkInfo = parsedUri->GetChunkInfo();
        const auto container = chunkInfo.Container;
        TMuxerFuture muxerFuture;
        switch (container) {
            case EContainer::MP4:
                muxerFuture = TMuxerMP4::Make(*this, drmInfo.Info4Muxer, /*contentLengthPromise = */ allowEarlyHeaders);
                break;
            case EContainer::TS:
                Y_ENSURE_EX(chunkInfo.Number > 0, THttpError(404, TLOG_WARNING) << "wrong chunk number for ts container: " << chunkInfo.Number);
                muxerFuture = TMuxerMpegTs::Make(*this, ui64(chunkInfo.Number - 1), drmInfo.Info4Muxer);
                break;
            default:
                Y_ENSURE(false, "failed to create muxer: container '" << container << "' unsupported");
        }

        const auto senderFuture = TSender::Make(*this, drmInfo.Info4Sender);

        const auto resultFuture = PackagerWaitExceptionOrAll(TVector{
            sourceFuture.IgnoreResult(),
            muxerFuture.IgnoreResult(),
            senderFuture.IgnoreResult(),
        });

        resultFuture.Subscribe(MakeFatalOnException(
            [this, sourceFuture, muxerFuture, senderFuture, parsedUri](const NThreading::TFuture<void>& resultFuture) {
                resultFuture.TryRethrow();

                const auto source = sourceFuture.GetValue();
                const auto muxer = muxerFuture.GetValue();
                const auto sender = senderFuture.GetValue();

                SetContentType(muxer->GetContentType(source->GetTracksInfo()));

                const auto chunkInfo = parsedUri->GetChunkInfo();
                if (chunkInfo.IsInitSegment) {
                    LogInfo() << "send init chunk";
                    DisableChunkedTransferEncoding();
                    SendData({muxer->MakeInitialSegment(source->GetTracksInfo()), false});
                    Finish();
                    return;
                }

                Y_ENSURE(SegmentInterval.Defined());
                const TIntervalP interval = SegmentInterval.GetRef();

                LogInfo() << "start prepare chunk:"
                          << " chunk_number=" << chunkInfo.Number
                          << " begin=" << interval.Begin
                          << " end=" << interval.End;

                auto mediaData = source->GetMedia(interval, Ti64TimeP(0));

                auto isCmaf = IsCmafEnabled(chunkInfo.Container);
                GetMetrics().IsCmaf = isCmaf ? 1 : 0;
                if (isCmaf) {
                    // TODO: enable partial source data loading in cmaf mode
                    // TODO: configure cmaf fragment size
                    const i64 desiredCmafSize = 500;
                    const i64 intervalDuration = (interval.End - interval.Begin).MilliSecondsCeil();
                    const i64 count = Max<i64>(1, DivFloor(intervalDuration, desiredCmafSize));
                    const i64 cmafSize = DivCeil(intervalDuration, count);

                    mediaData = TFragmentCutter::Cut(mediaData, Ms2P(cmafSize), *this);
                } else {
                    DisableChunkedTransferEncoding();
                    mediaData = TFragmentCutter::Cut(mediaData, interval.End - interval.Begin, *this);
                }

                muxer->SetMediaData(mediaData);

                sender->SetData(muxer->GetData());
            }));
    }

    TString TVodWorker::GetDescriptionUri(TString uri) const {
        return TStringBuilder() << Config.MetaLocation.GetRef() << uri;
    }

    TString TVodWorker::GetContentUri(TString uri) const {
        return TStringBuilder() << GetComplexValue(Config.ContentLocation.GetRef()) << uri;
    }

    bool TVodWorker::IsCmafEnabled(EContainer container) const {
        if (container != EContainer::MP4) {
            return false;
        }

        const auto clientBufferSize = GetArg<float>("bufsize", false);
        if (!clientBufferSize.Defined()) {
            return false;
        }

        // TODO: think about max buffer size configuration
        return clientBufferSize.GetRef() < 5;
    }

    Ti64TimeMs TVodWorker::GetChunkDuration(const NDescription::TDescription* description) const {
        if (description->SegmentDuration() > 0) {
            return Ti64TimeMs(description->SegmentDuration());
        }

        Y_ENSURE(Config.ChunkDuration.Defined());
        return *Config.ChunkDuration;
    }

    TVector<TSourceFuture> TVodWorker::MakeSources(const TVector<NDescription::TSourceInfo>& sourceInfos,
                                                   const TSourceMp4File::TConfig& mp4SourceConfig) {
        auto sourceFutures = TVector<TSourceFuture>();
        for (const auto& sourceInfo : sourceInfos) {
            LogInfo() << "make source:"
                      << " begin=" << sourceInfo.Interval.Begin
                      << " end=" << sourceInfo.Interval.End
                      << " offset=" << sourceInfo.Offset
                      << " path=" << sourceInfo.Path;

            sourceFutures.emplace_back(TSourceMp4File::Make(
                *this,
                mp4SourceConfig,
                GetContentUri(sourceInfo.Path),
                "",
                (TIntervalP)sourceInfo.Interval,
                (Ti64TimeP)sourceInfo.Offset));
        }
        return sourceFutures;
    }
}
