#include <util/generic/vector.h>
#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/common/muxer_caf.h>
#include <nginx/modules/strm_packager/src/common/sender.h>
#include <nginx/modules/strm_packager/src/common/source_tracks_select.h>
#include <nginx/modules/strm_packager/src/content/music_uri.h>
#include <nginx/modules/strm_packager/src/content/vod_description.h>
#include <nginx/modules/strm_packager/src/workers/music_worker.h>
#include <nginx/modules/strm_packager/src/common/hmac.h>
#include <nginx/modules/strm_packager/src/common/encrypting_buffer.h>
#include <util/stream/format.h>

namespace NStrm::NPackager {
    using NDescription::TDescription;
    using NThreading::TFuture;

    // the key is different every time so a zero iv suffices
    static TString MakeIv() {
        TString result;
        result.resize(TEncryptingBufferWriter::CipherBlockSize, 0);
        return result;
    }

    TMusicWorker::TMusicWorker(TRequestContext& context, const TLocationConfig& config)
        : TRequestWorker(context, config, /* kaltura mode = */ true, "music_worker:")
        , iv(MakeIv())
    {
    }

    void TMusicWorker::CheckConfig(const TLocationConfig& config) {
        TSourceMp4File::TConfig(config).Check();
        Y_ENSURE(config.MetaLocation.Defined());
        Y_ENSURE(config.ContentLocation.Defined());
    }

    TString TMusicWorker::GetContentUri(const TString& uri) const {
        return TStringBuilder() << GetComplexValue(Config.ContentLocation.GetRef()) << uri;
    }

    TString TMusicWorker::GetMetaUri(const TString& uri) const {
        return TStringBuilder() << Config.MetaLocation.GetRef() << uri;
    }

    void TMusicWorker::Work() {
        const auto uri = Config.URI.Defined()
                             ? GetComplexValue(Config.URI.GetRef())
                             : GetUri();

        const auto parsedUri = GetPoolUtil<TMusicUri>().New(uri);

        // check that the request is indeed valid
        if (!parsedUri->IsValid(*this, Config.SignSecret.GetOrElse(""))) {
            ythrow THttpError(403, TLOG_WARNING);
        }

        const auto descUri = GetMetaUri(parsedUri->GetDescriptionUri());
        LogDebug() << "url parsed: desc_uri=" << descUri;

        const auto descriptionGetter = [this, descUri]() {
            return CreateSubrequest(
                TSubrequestParams{
                    .Uri = descUri,
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

        TFuture<const TDescription*> descriptionFuture;
        if (!Config.DescriptionShmCacheZone.Defined()) {
            descriptionFuture = descriptionGetter().Apply(
                [this, descriptionSaver](const NThreading::TFuture<TBuffer>& future) mutable {
                    const auto buffer = descriptionSaver(future.GetValue());
                    return NFb::GetTDescription(HangDataInPool(std::move(buffer)).begin());
                });
        } else {
            descriptionFuture = Config.DescriptionShmCacheZone->Data().Get<const NDescription::TDescription*, TBuffer, TBuffer>(
                *this,
                descUri,
                descriptionGetter,
                descriptionLoader,
                descriptionSaver);
        }

        const TFuture<TString> sourcePath = descriptionFuture.Apply(
            [this](const TFuture<const TDescription*>& descFuture) -> TString {
                const auto& description = descFuture.GetValue();

                auto audio = NDescription::GetAudioTrack(description, 1);
                auto interval = TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(audio->Duration())};

                TVector<NDescription::TSourceInfo> sourceInfos;
                NDescription::AddSourceInfos(audio->Segments(), interval, /* offset = */ Ti64TimeMs(0), sourceInfos);
                Y_ENSURE(sourceInfos.size() == 1);

                return GetContentUri(sourceInfos[0].Path);
            });

        const TString key = NHmac::Hmac(NHmac::Md5,
                                        Config.EncryptSecret.GetOrElse(""),
                                        TString(parsedUri->GetKeyIv()));

        switch (parsedUri->GetContainer()) {
            case TMusicUri::Caf:
                WorkCaf(sourcePath, key);
                break;
            case TMusicUri::Mp4:
                WorkMp4(sourcePath, key);
                break;
            default:
                Y_ENSURE(false, "Unhandled music container type");
        }
    }

    void TMusicWorker::WorkCaf(const TFuture<TString>& sourcePath, const TString& key) {
        const TFuture<ISource*> sourceFuture = sourcePath.Apply(
            [this](const TFuture<TString>& sourcePath) -> TSourceFuture {
                const TSourceMp4File::TConfig mp4SourceConfig(Config);
                return TSourceMp4File::Make(
                    *this,
                    mp4SourceConfig,
                    sourcePath.GetValue(),
                    "");
            });

        const auto muxerFuture = TMuxerCaf::Make(*this, key, iv);
        const auto senderFuture = TSender::Make(*this);

        const auto wait = PackagerWaitExceptionOrAll(TVector{
            sourceFuture.IgnoreResult(),
            muxerFuture.IgnoreResult(),
            senderFuture.IgnoreResult(),
        });

        wait.Subscribe(MakeFatalOnException(
            [sourceFuture, muxerFuture, senderFuture](const NThreading::TFuture<void>& wait) {
                wait.TryRethrow();

                const auto source = sourceFuture.GetValue();
                const auto tracks = source->GetTracksInfo();
                const auto muxer = muxerFuture.GetValue();

                muxer->SetMediaData(source->GetMedia());

                const auto sender = senderFuture.GetValue();
                sender->SetData(muxer->GetData());
            }));
    }

    void TMusicWorker::WorkMp4(const TFuture<TString>& sourcePath, const TString& key) {
        const TFuture<TBuffer> dataFuture = sourcePath.Apply(
            [this](const TFuture<TString>& sourcePath) {
                return CreateSubrequest(
                    TSubrequestParams{
                        .Uri = sourcePath.GetValue(),
                        .Args = "",
                    },
                    NGX_HTTP_OK);
            });

        dataFuture.Subscribe(MakeFatalOnException(
            [this, key](const TFuture<TBuffer>& dataFuture) {
                const auto& data = dataFuture.GetValue();

                TEncryptingBufferWriter writer(*this, key, iv, 0);
                writer.WorkIO(data.Data(), data.Size());

                SendData(TSendData{
                    .Blob = HangDataInPool(std::move(writer.Buffer())),
                    .Flush = true,
                });
                Finish();
            }));
    }
}
