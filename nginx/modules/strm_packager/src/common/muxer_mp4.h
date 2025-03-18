#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/cenc_cipher.h>
#include <nginx/modules/strm_packager/src/common/drm_info_util.h>
#include <nginx/modules/strm_packager/src/common/h2645_util.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>

#include <util/generic/deque.h>

namespace NStrm::NPackager {
    class TMuxerMP4: public IMuxer {
    public:
        enum class EProtectionScheme: ui32 {
            UNSET = 0,
            CENC = 'cenc', // AES-CTR scheme
            CBC1 = 'cbc1', // AES-CBC scheme
            CENS = 'cens', // AES-CTR subsample pattern encryption scheme
            CBCS = 'cbcs', // AES-CBC subsample pattern encryption scheme
        };

    public:
        TMuxerMP4(
            TRequestWorker& request,
            const TMaybe<TDrmInfo>& drmFuture,
            const bool contentLengthPromise = true,
            const bool addServerTimeUuidBoxes = false);

        static TMuxerFuture Make(
            TRequestWorker& request,
            const NThreading::TFuture<TMaybe<TDrmInfo>>& drmFuture = NThreading::MakeFuture<TMaybe<TDrmInfo>>({}),
            const bool contentLengthPromise = true,
            const bool addServerTimeUuidBoxes = false);

        TSimpleBlob MakeInitialSegment(const TVector<TTrackInfo const*>& tracksInfo) const override;
        void SetMediaData(TRepFuture<TMediaData> data) override;
        TRepFuture<TSendData> GetData() override;
        TString GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const override;

    private:
        struct TPatternEncryption {
            ui32 CryptBlock;
            ui32 SkipBlock;
        };

        struct TProtectionData {
            ui32 AuxInfoType;
            ui32 AuxInfoTypeParameter;
            EProtectionScheme Scheme;
            TMaybe<TPatternEncryption> Pattern;
            size_t BlockSize;
            size_t IVSize;
            TMaybe<TString> IV;
            bool ConstantIV;
        };

        struct TProtectionState {
            TProtectionData Data; // iv must be set
            THolder<TCencCipher> Cipher;

            TTrackInfo const* TrackInfo;
            TVector<Nh2645::TAvcSpsParsed> ParsedAvcSps;
            TVector<Nh2645::TAvcPpsParsed> ParsedAvcPps;
            TVector<Nh2645::THevcSpsParsed> ParsedHevcSps;
            TVector<Nh2645::THevcPpsParsed> ParsedHevcPps;
        };

    private:
        // Callback on samples that doesnt have data yet (only data size known)
        //   to be able to amke moof boxes and send first moof box now waiting for samples subrequests to finish.
        // Doesnt work with DRM, since moof will depend on samples data.
        void MediaDataCallbackEmptyMode(const TRepFuture<TMediaData>::TDataRef&);

        void MediaDataCallback(const TRepFuture<TMediaData>::TDataRef&);

        static TProtectionData MakeProtectionData(
            const TDrmInfo& drmInfo,
            const EProtectionScheme scheme,
            const bool isNaluVideo);

        void ProtectMoov(NMP4Muxer::TMovieBox& moov, const TVector<TTrackInfo const*>& tracksInfo) const;

        void PrepareProtectionStates(const TVector<TTrackInfo const*>& tracksInfo, const Ti64TimeP beginTs);

        // replace data pointers in tracks of [trackBegin, trackEnd) with encrypted data blobs and fill auxInfo
        void ProtectTrack(TProtectionState& state, TSampleData* const trackBegin, TSampleData* const trackEnd, NMP4Muxer::TTrackProtectionAuxInfo& auxInfo);

        // return empty if data is not video slice nal, or numer of first bytes, that contain nal type byte and slice header
        static TMaybe<ui32> GetSliceDataOffsetAvc(TProtectionState& state, ui8 const* const data, const size_t dataSize);
        static TMaybe<ui32> GetSliceDataOffsetHevc(TProtectionState& state, ui8 const* const data, const size_t dataSize);

    private:
        TRequestWorker& Request;

        const TMaybe<TDrmInfo> DrmInfo;
        const EProtectionScheme ProtectionScheme;

        TVector<TProtectionState> TracksProtectionState; // use if drm enabled

        const bool ContentLengthPromise;
        const bool AddServerTimeUuidBoxes;

        TRepPromise<TSendData> DataPromise;
        ui32 MoofSequenceNumber;

        // ISO 14496-15 say in description of lengthSizeMinusOne
        // > The value of this field shall be one of 0, 1, or 3 corresponding to a length encoded with 1, 2, or 4 bytes, respectively.
        // So 4 is expected in all data, since 2 or 1 bytes length must be too small
        static constexpr int ExpectedNaluSizeLength = 4;

        struct TPreparedMoof {
            TIntervalP Interval;
            TSimpleBlob Moof;
            ui64 MdatSize;
            bool Sent;
            ui32 MoofSequenceNumber;
        };

        TDeque<TPreparedMoof> PreparedMoofs;
        ui32 PreparedMoofSequenceNumber;
        bool AllMoofsPrepared;
    };

    class TServerTimeUuidBox: NMP4Muxer::TBox {
    private:
        static constexpr TExtendedType uuids[2] = {
            {0xc4, 0xea, 0xc1, 0xfc, 0x0a, 0xd8, 0x46, 0x76, 0x96, 0x56, 0x69, 0x55, 0x2e, 0xb3, 0x25, 0x2f},
            {0xbc, 0xfc, 0xa2, 0x53, 0x39, 0x8b, 0x4a, 0x5b, 0x9a, 0x92, 0xec, 0x8f, 0x7d, 0x89, 0xe6, 0x1d},
        };

    public:
        TServerTimeUuidBox(bool withHeaders) {
            Type = 'uuid';
            std::memcpy(ExtendedType, withHeaders ? uuids[0] : uuids[1], sizeof(TExtendedType));
        }

        template <typename TWriter>
        void Write(TWriter& writer) {
            static_assert(TWriter::IsWriter());

            const ui64 beginPosition = writer.GetPosition();
            TBox::WorkIO(writer);
            WriteUi64BigEndian(writer, TInstant::Now().MilliSeconds());
            WorkIOActualSize(writer, beginPosition);
        }
    };

}
