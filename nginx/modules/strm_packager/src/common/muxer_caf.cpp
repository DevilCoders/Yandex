#include <util/generic/vector.h>
#include <nginx/modules/strm_packager/src/common/encrypting_buffer.h>
#include <nginx/modules/strm_packager/src/common/muxer_caf.h>
#include <strm/media/transcoder/mp4muxer/io.h>

namespace NStrm::NPackager {
    using namespace NMP4Muxer;

    enum {
        kCAFChannelLayoutTag_Stereo = (101 << 16) | 2, // a standard stereo stream (L R)
    };

    TMuxerCaf::TMuxerCaf(TRequestWorker& request, const TString& key, const TString& iv)
        : Request(request)
        , WasCalled(false)
        , Key(key)
        , IV(iv)
    {
    }

    TMuxerFuture TMuxerCaf::Make(TRequestWorker& request, const TString& key, const TString& iv) {
        TMuxerCaf* muxer = request.GetPoolUtil<TMuxerCaf>().New(request, key, iv);
        return NThreading::MakeFuture<IMuxer*>(muxer);
    }

    TSimpleBlob TMuxerCaf::MakeInitialSegment(const TVector<TTrackInfo const*>& tracksInfo) const {
        (void)tracksInfo;
        ythrow THttpError(NGX_HTTP_NOT_FOUND, TLOG_INFO);
    };

    TRepFuture<TSendData> TMuxerCaf::GetData() {
        Y_ENSURE(DataPromise.Initialized());
        return DataPromise.GetFuture();
    }

    void TMuxerCaf::SetMediaData(TRepFuture<TMediaData> data) {
        Y_ENSURE(!DataPromise.Initialized());
        DataPromise = TRepPromise<TSendData>::Make();
        RequireData(data, Request).AddCallback(Request.MakeFatalOnException(std::bind(&TMuxerCaf::MediaDataCallback, this, std::placeholders::_1)));
    }

    void TMuxerCaf::MediaDataCallback(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (dataRef.Empty()) {
            Request.LogDebug() << "MediaDataCallback empty";
            DataPromise.Finish();
            return;
        }

        Request.LogDebug() << "MediaDataCallback";

        // We need to ensure we were not called twice as there can be only one set of headers
        Y_ENSURE(WasCalled == false);
        WasCalled = true;

        const TMediaData& data = dataRef.Data();

        const TVector<TVector<TSampleData>> tracks = data.TracksSamples;
        Y_ENSURE(tracks.size() == 1);

        const TVector<const TTrackInfo*> infos = data.TracksInfo;
        Y_ENSURE(infos.size() == 1);

        const TVector<TSampleData>& track = tracks[0];
        Y_ENSURE(track.size() > 2);

        const TTrackInfo* info = infos[0];
        Y_ENSURE(std::holds_alternative<TTrackInfo::TAudioParams>(info->Params));

        const TTrackInfo::TAudioParams& ap = std::get<TTrackInfo::TAudioParams>(info->Params);
        Y_ENSURE(ap.BoxType == 'Opus');

        /*
         * The first several frames are used for opus codec adjustment and have
         * Dts < 0. We need to get the first actual data packet to get the
         * correct framesPerPacket size.
         */
        ui64 framesPerPacket = 0;
        for (auto& s : track) {
            if (s.Dts.Value >= 0) {
                framesPerPacket = Ti64TimeP(s.CoarseDuration).ConvertToTimescale(ap.SampleRate);
                break;
            }
        }
        Y_ENSURE(framesPerPacket > 0);

        TEncryptingBufferWriter writer(Request,
                                       Key,
                                       IV,
                                       0);

        // Opuses are always two-channel vbr
        AddCaffHeader(writer);

        AddDescHeader(writer,
                      ap.SampleRate,
                      'opus',
                      0,
                      0,
                      framesPerPacket,
                      ap.ChannelCount,
                      0);

        AddChanHeader(writer, kCAFChannelLayoutTag_Stereo, 0);

        ui64 totalSize = AddPaktHeader(writer, track, ap.SampleRate) + 4;
        writer.WorkIO("data", 4);

        // total size
        WriteUi64BigEndian(writer, totalSize);

        // edit count
        WriteUi32BigEndian(writer, (ui32)0);

        // copy the data
        for (const auto& s : track) {
            writer.WorkIO(s.Data, s.DataSize);
        }

        Request.LogDebug() << "Sending out " << writer.Buffer().size() << " bytes, " << track.size() << " packets";
        DataPromise.PutData(TSendData{Request.HangDataInPool(std::move(writer.Buffer())), false});
        Request.LogDebug() << "MediaDataCallback finished";
    }

    void TMuxerCaf::AddCaffHeader(TEncryptingBufferWriter& writer) const {
        writer.WorkIO("caff", 4);
        WriteUi32BigEndian(writer, (ui32)0x00010000); // version = 1, flags = 0
    }

    void TMuxerCaf::AddChanHeader(TEncryptingBufferWriter& writer,
                                  ui32 channelLayoutTag,
                                  ui32 channelBitmap) const {
        writer.WorkIO("chan", 4);

        // total length
        WriteUi64BigEndian(writer, 12);

        // the channel layout tag
        WriteUi32BigEndian(writer, channelLayoutTag);

        // the channel bitmap
        WriteUi32BigEndian(writer, channelBitmap);

        // number of channel descriptions
        WriteUi32BigEndian(writer, (ui32)0);
    }

    void TMuxerCaf::AddDescHeader(TEncryptingBufferWriter& writer,
                                  double sampleRate,
                                  ui32 formatId,
                                  ui32 formatFlags,
                                  ui32 bytesPerPacket,
                                  ui32 framesPerPacket,
                                  ui32 channelsPerFrame,
                                  ui32 bitsPerChannel) const {
        writer.WorkIO("desc", 4);

        // total length
        WriteUi64BigEndian(writer, 32);

        // sample rate
        WriteDoubleBigEndian(writer, sampleRate);

        // format id
        WriteUi32BigEndian(writer, formatId);

        // format flags
        WriteUi32BigEndian(writer, formatFlags);

        // bytes per packet
        WriteUi32BigEndian(writer, bytesPerPacket);

        // frames per packet
        WriteUi32BigEndian(writer, framesPerPacket);

        // channels per frame
        WriteUi32BigEndian(writer, channelsPerFrame);

        // bits per channel
        WriteUi32BigEndian(writer, bitsPerChannel);
    }

    ui64 TMuxerCaf::AddPaktHeader(TEncryptingBufferWriter& writer,
                                  const TVector<TSampleData>& track,
                                  ui64 sampleRate) const {
        // calculate the dynamic part size first
        ui64 dynamicSize = 0;
        for (const auto& s : track) {
            if (s.DataSize <= 127) {
                dynamicSize += 1;
                continue;
            }

            /*
             * Currently opusenc only supports frames up to 60 ms in size.
             * With the maximum bitrate of 256 kbit/s (== 32kbytes / second) it is
             *   32 * 1024 / 1000 * 60 = 1966
             * bytes per frame, so it is less than 14 bits
             */
            Y_ENSURE(s.DataSize < 16384 /* == 2 ^ 14*/);
            dynamicSize += 2;
        }

        Y_ENSURE(track.size() > 0);
        auto first = track.begin();
        auto last = track.rbegin();
        Ti64TimeP totalFramesDuration = last->Dts + last->CoarseDuration - first->Dts;

        writer.WorkIO("pakt", 4);

        /*
         * total size. We choose static size rather than dynamic so that we know precicely where
         * the header ends and data starts, so we can properly react to range-requests.
         * The static size assumes we use 2 bytes per packet (we can use either 1 or 2).
         */
        ui64 staticSize = track.size() * 2;
        WriteUi64BigEndian(writer, 8 + 8 + 4 + 4 + staticSize);

        // number of blocks
        WriteUi64BigEndian(writer, track.size());

        // number of valid frames
        WriteUi64BigEndian(writer, totalFramesDuration.ConvertToTimescale(sampleRate));

        // number of priming frames + number of remainder frames
        WriteUi64BigEndian(writer, 0);

        // sizes
        i64 totalSize = 0;
        ui8 c;
        for (const auto& s : track) {
            if (s.DataSize > 127) {
                c = ((s.DataSize >> 7u) & 0x7Fu) | 0x80u;
                writer.WorkIO(&c, 1);
            }

            c = s.DataSize & 0x7Fu;
            writer.WorkIO(&c, 1);

            totalSize += s.DataSize;
        }

        // pad up to static size
        c = 0;
        for (ui64 i = dynamicSize; i < staticSize; ++i) {
            writer.WorkIO(&c, 1);
        }

        return totalSize;
    }

    TString TMuxerCaf::GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const {
        return IMuxer::GetContentType(tracksInfo) + "/x-caf";
    }
}
