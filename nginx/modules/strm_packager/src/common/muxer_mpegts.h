#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/drm_info_util.h>
#include <nginx/modules/strm_packager/src/common/evp_cipher.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>

#include <util/generic/deque.h>

namespace NStrm::NPackager {
    class TMuxerMpegTs: public IMuxer {
    public:
        TMuxerMpegTs(TRequestWorker& request, ui64 chunkNumber, const TMaybe<TDrmInfo>& drmInfo);

        static TMuxerFuture Make(TRequestWorker& request, ui64 chunkNumber = 0, const NThreading::TFuture<TMaybe<TDrmInfo>>& drmFuture = NThreading::MakeFuture<TMaybe<TDrmInfo>>({}));

        TSimpleBlob MakeInitialSegment(const TVector<TTrackInfo const*>& tracksInfo) const override;
        void SetMediaData(TRepFuture<TMediaData> data) override;
        TRepFuture<TSendData> GetData() override;
        TString GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const override;

    public:
        // (aes 128 cbc)
        static constexpr int DrmKeySize = 16;
        static constexpr int DrmIVSize = 16;
        static constexpr int DrmBlockSize = 16;

        struct TStreamInfo {
            ui8 Type;
            TMaybe<TBuffer> ProgramDescriptors;
            TMaybe<TBuffer> ElementaryStreamDescriptors;
        };

    private:
        void MediaDataCallback(const TRepFuture<TMediaData>::TDataRef&);

        void AddFragment(const TMediaData& data);

        void AddPsi(const TVector<TStreamInfo>& streamsInfo);

        TSimpleBlob BlobFromHeaderTempBuffer();

        void AddDataToSend(const TBytesVector& data);
        void AddDataToSend(const TBytesChunk& data);
        void AddDataToSend(ui8 const* begin, ui8 const* end);

        // [begin; pos) - filled with data
        // [pos; end) - available
        struct TDataToSendBuffer {
            ui8* Begin;
            ui8* Pos;
            ui8* End;
        };

        TRequestWorker& Request;
        TRepPromise<TSendData> DataPromise;

        TVector<ui32> TrackContinuityCounters;

        TBytesVector HeaderTempBuffer;
        TDeque<TSimpleBlob> PayloadTempBuffer;

        TDeque<TDataToSendBuffer> DataToSend;
        size_t DataToSendNextBufferSize;

        ui64 ChunkNumber;
        bool PsiAdded;

        const ui32 PatPID;
        const ui32 PmtPID;
        const ui32 TracksPIDBegin;

        const TMaybe<TDrmInfo> DrmInfo;
    };
}
