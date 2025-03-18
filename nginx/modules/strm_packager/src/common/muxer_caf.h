#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/encrypting_buffer.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>

namespace NStrm::NPackager {
    class TMuxerCaf: public IMuxer {
    public:
        TMuxerCaf(TRequestWorker& request, const TString& key, const TString& iv);

        static TMuxerFuture Make(TRequestWorker& request, const TString& key, const TString& iv);

        TSimpleBlob MakeInitialSegment(const TVector<TTrackInfo const*>& tracksInfo) const override;
        void SetMediaData(TRepFuture<TMediaData> data) override;
        TRepFuture<TSendData> GetData() override;
        TString GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const override;

    private:
        void MediaDataCallback(const TRepFuture<TMediaData>::TDataRef&);

        void AddCaffHeader(TEncryptingBufferWriter& writer) const;

        void AddDescHeader(TEncryptingBufferWriter& writer,
                           double sampleRate,
                           ui32 formatId,
                           ui32 formatFlags,
                           ui32 bytesPerPacket,
                           ui32 framesPerPacket,
                           ui32 channelsPerFrame,
                           ui32 bitsPerChannel) const;

        void AddChanHeader(TEncryptingBufferWriter& writer,
                           ui32 channelLayoutTag,
                           ui32 channelBitmap) const;

        ui64 AddPaktHeader(TEncryptingBufferWriter& writer,
                           const TVector<TSampleData>& track,
                           ui64 sampleRate) const;

        TRequestWorker& Request;
        TRepPromise<TSendData> DataPromise;
        bool WasCalled;
        TString Key;
        TString IV;
    };
}
