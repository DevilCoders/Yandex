#include <nginx/modules/strm_packager/src/common/avcC_box_util.h>
#include <nginx/modules/strm_packager/src/common/h2645_util.h>
#include <nginx/modules/strm_packager/src/common/mp4_common.h>
#include <nginx/modules/strm_packager/src/common/muxer_mpegts.h>
#include <strm/media/formats/mpegts/mpegts.h>

#include <util/generic/overloaded.h>
#include <util/generic/queue.h>

namespace NStrm::NPackager {
    using TSection = NStrm::NMpegts::NPsi::TSection;
    using TPatSection = NStrm::NMpegts::NPsi::TPatSection;
    using TPmtSection = NStrm::NMpegts::NPsi::TPmtSection;
    using TTsPacket = NStrm::NMpegts::NTs::TPacket;
    using TPesPacket = NStrm::NMpegts::NPes::TPacket;

    using TStreamInfo = TMuxerMpegTs::TStreamInfo;

    using EAvcNalType = Nh2645::EAvcNalType;

    namespace {
        // (for debug) set to true to generate (almost) the same chunks as from kaltura
        constexpr bool DebugTsAsKaltura = false;

        constexpr int ProgramNumber = 1;

        class IPayloadMaker {
        public:
            virtual ~IPayloadMaker() = default;
            virtual void MakePayload(const TSampleData& s, TDeque<TSimpleBlob>& payload) const = 0;
            virtual TStreamInfo StreamInfo() const = 0;
        };

        class TVideoAvcAnnexbPayloadMaker: public IPayloadMaker {
        public:
            TVideoAvcAnnexbPayloadMaker(const TTrackInfo::TVideoParams& vp, TRequestWorker& request, const TMaybe<TDrmInfo>& drmInfo)
                : Request(request)
                , SpsPps(NAvcCBox::GetSpsPps(vp.CodecParamsBox->Data))
                , DrmInfo(drmInfo)
            {
                if (DrmInfo.Defined()) {
                    DrmCipher.ConstructInPlace(EVP_aes_128_cbc());
                    Y_ENSURE(DrmCipher->KeyLength() == TMuxerMpegTs::DrmKeySize);
                    Y_ENSURE(DrmCipher->IVLength() == TMuxerMpegTs::DrmIVSize);
                    Y_ENSURE(DrmCipher->BlockSize() == TMuxerMpegTs::DrmBlockSize);
                }
            }

            TStreamInfo StreamInfo() const override {
                if (DrmInfo.Empty()) {
                    return TStreamInfo{TPmtSection::VideoAvcAnnexB, {}, {}};
                }

                TStreamInfo result;
                result.Type = TPmtSection::VideoAvcAnnexBSampleAes128;

                NMP4Muxer::TBufferWriter writer;

                // private_data_indicator_descriptor
                WriteUI8(writer, 15);                     // tag
                WriteUI8(writer, 4);                      // size = 4
                WriteUi32BigEndian(writer, (ui32)'zavc'); // = private_data_indicator

                result.ElementaryStreamDescriptors = std::move(writer.Buffer());

                return result;
            }

            void MakePayload(const TSampleData& s, TDeque<TSimpleBlob>& payload) const override {
                // access unit delimiter nal
                static const ui8 accessUnitDelimeter[] = {0x00, 0x00, 0x00, 0x01, 0x09, 0xf0};
                static const ui8 marker[] = {0x00, 0x00, 0x00, 0x01};

                payload.clear();

                payload.push_back(TSimpleBlob(accessUnitDelimeter, sizeof(accessUnitDelimeter)));

                if (s.IsKeyframe()) {
                    for (const TSimpleBlob& sps : SpsPps.Sps) {
                        payload.push_back(TSimpleBlob(marker, sizeof(marker)));
                        payload.push_back(sps);
                    }
                    for (const TSimpleBlob& pps : SpsPps.Pps) {
                        payload.push_back(TSimpleBlob(marker, sizeof(marker)));
                        payload.push_back(pps);
                    }
                }

                if (s.DataParams.Format == TSampleData::TDataParams::EFormat::VideoAvcc) {
                    Y_ENSURE(s.DataParams.NalUnitLengthSize.Defined());
                    const ui32 lengthSize = *s.DataParams.NalUnitLengthSize;
                    ui8 const* data = s.Data;
                    ui32 dataSize = s.DataSize;

                    while (dataSize > 0) {
                        Y_ENSURE(dataSize >= lengthSize);
                        ui32 tmpLength = 0;
                        for (ui32 i = 0; i < lengthSize; ++i) {
                            tmpLength = (tmpLength << 8) | ui32(*data);
                            ++data;
                            --dataSize;
                        }

                        const ui32 length = tmpLength;
                        Y_ENSURE(length <= dataSize);
                        Y_ENSURE(length >= 1);

                        const EAvcNalType naluType = EAvcNalType(data[0] & 0b00011111);

                        if (naluType == EAvcNalType::AUD) {
                            // skip access unit delimeters if present in original bitstream
                        } else if (DrmCipher.Defined() && (naluType == EAvcNalType::SLICE || naluType == EAvcNalType::IDR_SLICE) && length > 48) {
                            // encrypt for FairPlay
                            DrmCipher->EncryptInit((ui8 const*)DrmInfo->Key.Data(), (ui8 const*)DrmInfo->IV->Data(), /*padding = */ false);

                            ui8 encData[length];
                            {
                                const int unencLeaderSize = 32;
                                const int encSize = 16;
                                const int unencSize = 144;

                                ui8* encPtr = encData;
                                ui8 const* begin = data;
                                ui8 const* const end = data + length;

                                std::memcpy(encPtr, begin, unencLeaderSize);
                                encPtr += unencLeaderSize;
                                begin += unencLeaderSize;

                                while (begin < end) {
                                    if (end - begin > encSize) {
                                        Y_ENSURE(encSize == DrmCipher->EncryptUpdate(encPtr, begin, encSize));
                                        begin += encSize;
                                        encPtr += encSize;
                                    }

                                    const int copySize = Min<int>(end - begin, unencSize);
                                    std::memcpy(encPtr, begin, copySize);
                                    encPtr += copySize;
                                    begin += copySize;
                                }

                                Y_ENSURE(0 == DrmCipher->EncryptFinal(encData));

                                Y_ENSURE(begin == end);
                                Y_ENSURE(encPtr == encData + length);
                            }

                            // copy encData to epData applying start code emulation prevention
                            ui8 epData[length * 3 / 2 + 1];
                            int epDataSize = 0;

                            int zerosCount = 0;
                            for (const ui8 b : encData) {
                                if (zerosCount >= 2 && (b == 0 || b == 1 || b == 2 || b == 3)) {
                                    epData[epDataSize] = 3;
                                    ++epDataSize;
                                    zerosCount = 0;
                                }

                                epData[epDataSize] = b;
                                ++epDataSize;
                                if (b == 0) {
                                    ++zerosCount;
                                } else {
                                    zerosCount = 0;
                                }
                            }

                            ui8* const epDataCopy = Request.MemoryPool.AllocateArray<ui8>(epDataSize, /*align = */ 1);

                            std::memcpy(epDataCopy, epData, epDataSize);

                            payload.push_back(TSimpleBlob(marker, sizeof(marker)));
                            payload.push_back(TSimpleBlob(epDataCopy, epDataSize));
                        } else {
                            payload.push_back(TSimpleBlob(marker, sizeof(marker)));
                            payload.push_back(TSimpleBlob(data, length));
                        }

                        data += length;
                        dataSize -= length;
                    }
                } else {
                    Y_ENSURE(false, "TVideoAvcAnnexbPayloadMaker::MakePayload: unsupported sample data format " << s.DataParams.Format);
                }
            }

        private:
            TRequestWorker& Request;
            const NAvcCBox::TSpsPpsRange SpsPps;
            const TMaybe<TDrmInfo> DrmInfo;
            mutable TMaybe<TEvpCipher> DrmCipher;
        };
    }

    static inline TBuffer MakeFairPlayAudioDescriptors(const ui32 privateDataIndicator, const ui32 audioType, const TBuffer& setupData) {
        NMP4Muxer::TBufferWriter writer;

        // private_data_indicator_descriptor
        WriteUI8(writer, 15);                             // tag
        WriteUI8(writer, 4);                              // size = 4
        WriteUi32BigEndian(writer, privateDataIndicator); // private_data_indicator

        // registration_descriptor
        WriteUI8(writer, 5); // tag
        const ui64 sizePos = writer.GetPosition();
        WriteUI8(writer, 0);                      // size will be set after
        WriteUi32BigEndian(writer, (ui32)'apad'); // format_identifier
        WriteUi32BigEndian(writer, audioType);    // audio_type
        WriteUi16BigEndian(writer, 0);            // priming
        WriteUI8(writer, 1);                      // version

        Y_ENSURE(setupData.Size() == (setupData.Size() & 0xff));
        WriteUI8(writer, setupData.Size()); // setup_data_len
        writer.WorkIO(setupData.Data(), setupData.Size());

        const ui64 size = writer.GetPosition() - sizePos - 1;
        Y_ENSURE((size & 0xff) == size);
        writer.SetPosition(sizePos);
        WriteUI8(writer, (size & 0xff)); // write actual size

        return std::move(writer.Buffer());
    }

    static inline void AudioFairPlayEncryptBlock(ui8 const* const data, const ui32 size, TEvpCipher& cipher, TRequestWorker& request, TDeque<TSimpleBlob>& payload) {
        const ui32 blockSize = TMuxerMpegTs::DrmBlockSize;

        if (size < blockSize * 2) {
            payload.push_back(TSimpleBlob(data, size));
        } else {
            // unencrypted_leader 16 bytes
            payload.push_back(TSimpleBlob(data, blockSize));

            // encrypted block
            const ui32 encSize = size - blockSize - (size % blockSize);
            Y_ENSURE(encSize > 0);

            ui8* const enc = request.MemoryPool.AllocateArray<ui8>(encSize, /*align = */ 1);

            Y_ENSURE(encSize == cipher.EncryptUpdate(enc, data + blockSize, encSize));

            payload.push_back(TSimpleBlob(enc, encSize));

            if (size > blockSize + encSize) {
                payload.push_back(TSimpleBlob(data + blockSize + encSize, size - blockSize - encSize));
            }
        }
    }

    class TAudioDrmEAC3PayloadMaker: public IPayloadMaker {
    public:
        TAudioDrmEAC3PayloadMaker(const TTrackInfo::TAudioParams& ap, TRequestWorker& request, const TDrmInfo& drmInfo)
            : Request(request)
            , DrmInfo(drmInfo)
            , DrmCipher(EVP_aes_128_cbc())
        {
            Y_ENSURE(ap.CodecType == TTrackInfo::TAudioParams::ECodecType::EAC3);
            Y_ENSURE(ap.EC3SpecificBox.Defined());
            EC3SpecificBoxData = ap.EC3SpecificBox->Data;

            Y_ENSURE(DrmCipher.KeyLength() == TMuxerMpegTs::DrmKeySize);
            Y_ENSURE(DrmCipher.IVLength() == TMuxerMpegTs::DrmIVSize);
            Y_ENSURE(DrmCipher.BlockSize() == TMuxerMpegTs::DrmBlockSize);
        }

        TStreamInfo StreamInfo() const override {
            TStreamInfo result;
            result.Type = TPmtSection::AudioEAc3SampleAes128;
            result.ElementaryStreamDescriptors = MakeFairPlayAudioDescriptors('ec3d', 'zec3', EC3SpecificBoxData);
            return result;
        }

        void MakePayload(const TSampleData& s, TDeque<TSimpleBlob>& payload) const override {
            payload.clear();

            Y_ENSURE(s.DataParams.Format == TSampleData::TDataParams::EFormat::AudioEAc3);

            DrmCipher.EncryptInit((ui8 const*)DrmInfo.Key.Data(), (ui8 const*)DrmInfo.IV->Data(), /*padding = */ false);

            ui8 const* data = s.Data;
            ui32 size = s.DataSize;

            // E.1.2.0 E-AC-3_bit_stream and syncframe [ETSI TS 102 366 V1.4.1]
            while (size > 0) {
                // read syncframe size
                Y_ENSURE(size > 4);
                const ui32 syncword = (ui32(data[0]) << 8) | data[1];
                Y_ENSURE(syncword == 0x0B77);
                const ui32 frmsiz = ((ui32(data[2]) & 0b00000111) << 8) | data[3];
                const ui32 frameSize = (frmsiz + 1) * 2;

                Y_ENSURE(size >= frameSize);

                // put syncframe encrypted
                AudioFairPlayEncryptBlock(data, frameSize, DrmCipher, Request, payload);

                data += frameSize;
                size -= frameSize;
            }
        }

    private:
        TRequestWorker& Request;
        const TDrmInfo DrmInfo;
        mutable TEvpCipher DrmCipher;
        TBuffer EC3SpecificBoxData;
    };

    class TAudioDrmAC3PayloadMaker: public IPayloadMaker {
    public:
        TAudioDrmAC3PayloadMaker(const TTrackInfo::TAudioParams& ap, TRequestWorker& request, const TDrmInfo& drmInfo, const TSampleData frameData)
            : Request(request)
            , DrmInfo(drmInfo)
            , DrmCipher(EVP_aes_128_cbc())
        {
            Y_ENSURE(ap.CodecType == TTrackInfo::TAudioParams::ECodecType::AC3);

            // make SetupData - it is first 10 bytes of a frame
            //  so it is required that at least one frame must be available
            Y_ENSURE(frameData.Data);
            Y_ENSURE(frameData.DataFuture.Initialized() && frameData.DataFuture.HasValue());
            SetupData.Assign((const char*)frameData.Data, Min(frameData.DataSize, 10u));

            Y_ENSURE(DrmCipher.KeyLength() == TMuxerMpegTs::DrmKeySize);
            Y_ENSURE(DrmCipher.IVLength() == TMuxerMpegTs::DrmIVSize);
            Y_ENSURE(DrmCipher.BlockSize() == TMuxerMpegTs::DrmBlockSize);
        }

        TStreamInfo StreamInfo() const override {
            TStreamInfo result;
            result.Type = TPmtSection::AudioAc3SampleAes128;
            result.ElementaryStreamDescriptors = MakeFairPlayAudioDescriptors('ac3d', 'zac3', SetupData);
            return result;
        }

        void MakePayload(const TSampleData& s, TDeque<TSimpleBlob>& payload) const override {
            payload.clear();

            Y_ENSURE(s.DataParams.Format == TSampleData::TDataParams::EFormat::AudioAc3);

            DrmCipher.EncryptInit((ui8 const*)DrmInfo.Key.Data(), (ui8 const*)DrmInfo.IV->Data(), /*padding = */ false);

            ui8 const* data = s.Data;
            const ui32 size = s.DataSize;

            // 4.3.0 AC-3_bit_stream and syncframe [ETSI TS 102 366 V1.4.1]
            Y_ENSURE(size > 5);
            Y_ENSURE(data[0] == 0x0B);
            Y_ENSURE(data[1] == 0x77);

            const ui32 fscod = (data[4] >> 6);
            const ui32 frmsizecod = (data[4] & 0b00111111);
            (void)fscod;
            (void)frmsizecod;
            // TODO: maybe compute size from fscod and frmsizecod and check it equal s.DataSize - 2, but do we need it?

            AudioFairPlayEncryptBlock(data, size, DrmCipher, Request, payload);
        }

    private:
        TRequestWorker& Request;
        const TDrmInfo DrmInfo;
        mutable TEvpCipher DrmCipher;
        TBuffer SetupData;
    };

    class TAudioAacAdtsPayloadMaker: public IPayloadMaker {
    public:
        TAudioAacAdtsPayloadMaker(const TTrackInfo::TAudioParams& ap, TRequestWorker& request, const TMaybe<TDrmInfo>& drmInfo)
            : Request(request)
            , DrmInfo(drmInfo)
        {
            switch (ap.CodecType) {
                case TTrackInfo::TAudioParams::ECodecType::AacMain:
                    ObjectType = 1;
                    break;
                case TTrackInfo::TAudioParams::ECodecType::AacLC:
                    ObjectType = 2;
                    break;
                case TTrackInfo::TAudioParams::ECodecType::AacSSR:
                    ObjectType = 3;
                    break;
                case TTrackInfo::TAudioParams::ECodecType::AacLTP:
                    ObjectType = 4;
                    break;

                default:
                    Y_ENSURE(false, "TAudioAacAdtsPayloadMaker: unsupported codec type " << ap.CodecType);
            }

            switch (ap.ChannelCount) {
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                    ChannelConfiguration = ap.ChannelCount;
                    break;
                case 8:
                    ChannelConfiguration = 7;
                    break;
                default:
                    Y_ENSURE(false, "TAudioAacAdtsPayloadMaker: unsupported number of channels " << ap.ChannelCount);
            }

            // Table 'Sampling Frequency Index' at ISO 14496-3
            const ui32 frequencies[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0};

            for (ui32 i = 0;; ++i) {
                Y_ENSURE(frequencies[i], "TAudioAacAdtsPayloadMaker: unsupported sample rate " << ap.SampleRate);
                if (frequencies[i] == ap.SampleRate) {
                    SamplingFrequencyIndex = i;
                    break;
                }
            }

            if (DrmInfo.Defined()) {
                DrmCipher.ConstructInPlace(EVP_aes_128_cbc());
                Y_ENSURE(DrmCipher->KeyLength() == TMuxerMpegTs::DrmKeySize);
                Y_ENSURE(DrmCipher->IVLength() == TMuxerMpegTs::DrmIVSize);
                Y_ENSURE(DrmCipher->BlockSize() == TMuxerMpegTs::DrmBlockSize);
            }

            DecoderSpecificInfo = ap.DecoderSpecificInfo;
        }

        TStreamInfo StreamInfo() const override {
            if (DrmInfo.Empty()) {
                return TStreamInfo{TPmtSection::AudioAacAdts, {}, {}};
            }

            TStreamInfo result;
            result.Type = TPmtSection::AudioAacAdtsSampleAes128;
            result.ElementaryStreamDescriptors = MakeFairPlayAudioDescriptors('aacd', 'zaac', DecoderSpecificInfo);
            return result;
        }

        void MakePayload(const TSampleData& s, TDeque<TSimpleBlob>& payload) const override {
            payload.clear();

            Y_ENSURE(s.DataParams.Format == TSampleData::TDataParams::EFormat::AudioAccRDB ||
                     s.DataParams.Format == TSampleData::TDataParams::EFormat::AudioAccADTS);

            if (s.DataParams.Format == TSampleData::TDataParams::EFormat::AudioAccRDB) {
                static const int headerSize = 7;
                const int frameSize = headerSize + s.DataSize;

                ui8* const adtsHeader = Request.MemoryPool.AllocateArray<ui8>(headerSize, /*align = */ 1);

                // header:
                // [0:] AAAAAAAA [1:] AAAABCCD [2:] EEFFFFGH [3:] HHIJKLMM [4:] MMMMMMMM [5:] MMMOOOOO [6;] OOOOOOPP

                // A: 12 bit of syncword: all 1
                // B: 1 bit of ID: 0 for MPEG-4
                // C: 2 bit of Layer: always 0
                // D: 1 bit of protection absent: 1, since no CRC
                adtsHeader[0] = 0b11111111;
                adtsHeader[1] = 0b11110001;

                // E: 2 bit of profile = ObjectType-1
                adtsHeader[2] = ((ObjectType - 1) & 0b11) << 6;

                // F: 4 bit sampling frequency index
                adtsHeader[2] |= (SamplingFrequencyIndex & 0b1111) << 2;

                // G: 1 bit - private bit = 0
                // H: 3 bit - channel configuration
                adtsHeader[2] |= (ChannelConfiguration & 0b100) >> 2;
                adtsHeader[3] = (ChannelConfiguration & 0b011) << 6;

                // I: 1 bit originality = 0
                // J: 1 bit home = 0
                // K: 1 bit copyright id bit = 0
                // L: 1 bit copyright id start = 0
                // M: 13 bit frame size
                Y_ENSURE(frameSize == (frameSize & 0b1111111111111), "TAudioAacAdtsPayloadMaker: too large frame");
                adtsHeader[3] |= (frameSize & 0b1100000000000) >> 11;
                adtsHeader[4] = (frameSize & 0b0011111111000) >> 3;
                adtsHeader[5] = (frameSize & 0b0000000000111) << 5;

                // M: 11 bit buffer fullness: all 1
                // P: 2 bit - Number of AAC frames (RDBs) in ADTS frame minus one: 0
                adtsHeader[5] |= 0b00011111;
                adtsHeader[6] = 0b11111100;

                payload.push_back(TSimpleBlob(adtsHeader, headerSize));
            }

            if (DrmCipher.Defined()) {
                // encrypt for FairPlay
                ui8 const* data = s.Data;
                ui32 size = s.DataSize;

                if (s.DataParams.Format == TSampleData::TDataParams::EFormat::AudioAccADTS) {
                    Y_ENSURE(size > 7); // ADTS header is at least 7 bytes
                    const ui32 headerSize = (data[1] & 1) ? 7 : 9;
                    Y_ENSURE(size > headerSize);

                    // unencrypted ADTS HEADER
                    payload.push_back(TSimpleBlob(data, headerSize));
                    data += headerSize;
                    size -= headerSize;
                }

                DrmCipher->EncryptInit((ui8 const*)DrmInfo->Key.Data(), (ui8 const*)DrmInfo->IV->Data(), /*padding = */ false);

                AudioFairPlayEncryptBlock(data, size, *DrmCipher, Request, payload);
            } else {
                payload.push_back(TSimpleBlob(s.Data, s.DataSize));
            }
        }

    private:
        TRequestWorker& Request;
        ui32 ObjectType;
        ui32 ChannelConfiguration;
        ui32 SamplingFrequencyIndex;
        TBuffer DecoderSpecificInfo;
        const TMaybe<TDrmInfo> DrmInfo;
        mutable TMaybe<TEvpCipher> DrmCipher;
    };

    class TAudioCommonPayloadMaker: public IPayloadMaker {
    public:
        TAudioCommonPayloadMaker(const TTrackInfo::TAudioParams& ap) {
            switch (ap.CodecType) {
                case TTrackInfo::TAudioParams::ECodecType::Mp3Mpeg1a:
                    StreamType_ = TPmtSection::AudioMpeg1a;
                    break;
                case TTrackInfo::TAudioParams::ECodecType::Mp3Mpeg2a:
                    StreamType_ = TPmtSection::AudioMpeg2a;
                    break;

                case TTrackInfo::TAudioParams::ECodecType::AC3:
                    StreamType_ = TPmtSection::AudioAc3EAc3;
                    break;

                case TTrackInfo::TAudioParams::ECodecType::EAC3:
                    // there is special type for eac3 only: TPmtSection::AudioEAc3, but kaltura use the same as ac3, so do the same
                    StreamType_ = TPmtSection::AudioAc3EAc3;
                    break;

                default:
                    Y_ENSURE(false, "TAudioCommonPayloadMaker: unsupported codec type " << ap.CodecType);
            }

            DataFormat = Mp4TrackParams2TrackDataParams(ap).Format;
        }

        TStreamInfo StreamInfo() const override {
            return TStreamInfo{StreamType_, {}, {}};
        }

        void MakePayload(const TSampleData& s, TDeque<TSimpleBlob>& payload) const override {
            Y_ENSURE(s.DataParams.Format == DataFormat);

            payload.clear();
            payload.push_back(TSimpleBlob(s.Data, s.DataSize));
        }

    private:
        TPmtSection::EStreamType StreamType_;
        TSampleData::TDataParams::EFormat DataFormat;
    };

    // descriptors values are from
    //   https://developer.apple.com/library/archive/documentation/AudioVideo/Conceptual/HTTP_Live_Streaming_Metadata_Spec/2/2.html
    //   also kaltura makes the same
    // descriptors structure - 'Metadata_pointer_descriptor ()' and 'Metadata_descriptor ()' of ISO/IEC 13818-1:2007
    class TTimedMetaId3PayloadMaker: public IPayloadMaker {
    public:
        TTimedMetaId3PayloadMaker() = default;

        void MakePayload(const TSampleData& s, TDeque<TSimpleBlob>& payload) const override {
            Y_ENSURE(s.DataParams.Format == TSampleData::TDataParams::EFormat::TimedMetaId3);
            payload.clear();
            payload.push_back(TSimpleBlob(s.Data, s.DataSize));
        }

        TStreamInfo StreamInfo() const override {
            static_assert(ProgramNumber == 1); // hardcoded in programDescriptor[] for now

            static const ui8 programDescriptor[] = {
                0x25,                   // descriptor_tag = 37 (decimal) - Metadata_pointer_descriptor tag
                0x0f,                   // descriptor_length
                0xff, 0xff,             // metadata_application_format = 0xffff
                0x49, 0x44, 0x33, 0x20, // metadata_format_identifier = 'ID3 '
                0xff,                   // metadata_format = 0xff
                0x49, 0x44, 0x33, 0x20, // metadata_format_identifier = 'ID3 '
                0x00,                   // metadata_service_id - any ID, typically 0
                0x1f,                   // - (1 bit) metadata_locator_record_flag = 0
                                        // - (2 bit) MPEG_carriage_flags = 0
                                        // - (5 bit) reserved = 0x1f
                0x00, 0x01,             // program_number = 1
            };

            static const ui8 elementaryStreamDescriptor[] = {
                0x26,                   // descriptor_tag - 38 (decimal)
                0x0d,                   // descriptor_length
                0xff, 0xff,             // metadata_application_format
                0x49, 0x44, 0x33, 0x20, // metadata_application_format_identifier = 'ID3 '
                0xff,                   // metadata_format
                0x49, 0x44, 0x33, 0x20, // metadata_format_identifier = 'ID3 '
                0x00,                   // metadata_service_id - any ID, typically 0
                0x0f,                   // - (3 bit) decoder_config_flags = 0
                                        // - (1 bit) DSM-CC_flag = 0
                                        // - (4 bit) reserved = 0xf
            };

            TStreamInfo result;
            result.Type = TPmtSection::EStreamType::MetadataInPES;
            result.ProgramDescriptors = TBuffer((char*)programDescriptor, sizeof(programDescriptor));
            result.ElementaryStreamDescriptors = TBuffer((char*)elementaryStreamDescriptor, sizeof(elementaryStreamDescriptor));
            return result;
        }
    };

    void TMuxerMpegTs::AddFragment(const TMediaData& data) {
        constexpr i64 mpegtsTimescale = 90000;
        constexpr i64 mpegtsPcrTimescale = 27000000;

        // prepare stream identifiers
        TVector<TPesPacket::EStreamIdentifier> track2streamid;
        track2streamid.reserve(data.TracksInfo.size());
        TPesPacket::EStreamIdentifier audioId = TPesPacket::EStreamIdentifier::AudioStream_First;
        TPesPacket::EStreamIdentifier videoId = TPesPacket::EStreamIdentifier::VideoStream_First;

        // and payload makers
        TVector<THolder<IPayloadMaker>> trackPayloadMakers;
        trackPayloadMakers.reserve(data.TracksInfo.size());

        for (size_t trackIndex = 0; trackIndex < data.TracksInfo.size(); ++trackIndex) {
            std::visit(
                TOverloaded{
                    [&](const TTrackInfo::TTimedMetaId3Params&) {
                        track2streamid.push_back(TPesPacket::EStreamIdentifier::PrivateStream1);
                        trackPayloadMakers.push_back(MakeHolder<TTimedMetaId3PayloadMaker>());
                    },
                    [&](const TTrackInfo::TAudioParams& ap) {
                        track2streamid.push_back(audioId);
                        audioId = TPesPacket::EStreamIdentifier(audioId + 1);

                        switch (ap.CodecType) {
                            case TTrackInfo::TAudioParams::ECodecType::AacMain:
                            case TTrackInfo::TAudioParams::ECodecType::AacLC:
                            case TTrackInfo::TAudioParams::ECodecType::AacSSR:
                            case TTrackInfo::TAudioParams::ECodecType::AacLTP:
                                trackPayloadMakers.push_back(MakeHolder<TAudioAacAdtsPayloadMaker>(ap, Request, DrmInfo));
                                break;

                            case TTrackInfo::TAudioParams::ECodecType::EAC3:
                                if (DrmInfo.Defined()) {
                                    trackPayloadMakers.push_back(MakeHolder<TAudioDrmEAC3PayloadMaker>(ap, Request, *DrmInfo));
                                    break;
                                }

                            case TTrackInfo::TAudioParams::ECodecType::AC3:
                                if (DrmInfo.Defined()) {
                                    Y_ENSURE(data.TracksSamples[trackIndex].size() > 0);
                                    trackPayloadMakers.push_back(MakeHolder<TAudioDrmAC3PayloadMaker>(ap, Request, *DrmInfo, data.TracksSamples[trackIndex].front()));
                                    break;
                                }

                            case TTrackInfo::TAudioParams::ECodecType::Mp3Mpeg1a:
                            case TTrackInfo::TAudioParams::ECodecType::Mp3Mpeg2a:
                                Y_ENSURE(!DrmInfo.Defined());
                                trackPayloadMakers.push_back(MakeHolder<TAudioCommonPayloadMaker>(ap));
                                break;

                            default:
                                Y_ENSURE(false, "TMuxerMpegTs::AddFragment: unsupported audio codec type " << ap.CodecType);
                        }
                    },
                    [&](const TTrackInfo::TVideoParams& vp) {
                        track2streamid.push_back(videoId);
                        videoId = TPesPacket::EStreamIdentifier(videoId + 1);

                        Y_ENSURE(vp.BoxType == 'avc1');

                        if (vp.CodecParamsBox->Type == 'avcC') {
                            trackPayloadMakers.push_back(MakeHolder<TVideoAvcAnnexbPayloadMaker>(vp, Request, DrmInfo));
                        } else {
                            Y_ENSURE(false, "unknown video codec params box");
                        }
                    },
                    [](const TTrackInfo::TSubtitleParams&) {
                        Y_ENSURE(false, "subtitles are not supported in mpegts");
                    }},
                data.TracksInfo[trackIndex]->Params);
        }

        // prepare bytes chain for ts
        TVector<ui32>& counters = TrackContinuityCounters;
        counters.resize(Max(counters.size(), data.TracksInfo.size()), 0);

        if (!PsiAdded) {
            TVector<TStreamInfo> streamsInfo;
            for (const auto& maker : trackPayloadMakers) {
                streamsInfo.emplace_back(maker->StreamInfo());
            }

            AddPsi(streamsInfo);
        }

        using TIndexPair = std::pair<size_t, size_t>;

        const auto compTrackIndex = [&data](const TIndexPair a, const TIndexPair b) -> bool {
            return data.TracksSamples[a.first][a.second].Dts > data.TracksSamples[b.first][b.second].Dts;
        };

        TPriorityQueue<TIndexPair, TVector<TIndexPair>, decltype(compTrackIndex)> tracksQueue(compTrackIndex);

        for (size_t trackIndex = 0; trackIndex < data.TracksSamples.size(); ++trackIndex) {
            if (!data.TracksSamples[trackIndex].empty()) {
                tracksQueue.push(TIndexPair(trackIndex, 0));
            }
        }

        constexpr size_t defaultTsPayloadSize = NMpegts::NTs::PayloadSizeMax;
        constexpr size_t defaultTsHeaderSize = NMpegts::NTs::HeaderSizeMin;
        constexpr size_t countersN = 16;
        TVector<std::array<std::array<ui8, defaultTsHeaderSize>, countersN>> DefaultTsHeaders(data.TracksSamples.size());

        for (size_t trackIndex = 0; trackIndex < data.TracksSamples.size(); ++trackIndex) {
            for (size_t counter = 0; counter < countersN; ++counter) {
                TTsPacket tsPacket;
                tsPacket.PacketIdentifier = TracksPIDBegin + trackIndex;
                tsPacket.ContinuityCounter = counter;

                MakeTsPacketHeader(tsPacket, defaultTsPayloadSize, /* force_payload_field_flag = */ true, HeaderTempBuffer);

                Y_ENSURE(HeaderTempBuffer.size() == defaultTsHeaderSize);

                for (size_t i = 0; i < defaultTsHeaderSize; ++i) {
                    DefaultTsHeaders[trackIndex][counter][i] = HeaderTempBuffer[i];
                }
            }
        }

        while (!tracksQueue.empty()) {
            const std::pair<size_t, size_t> cur = tracksQueue.top();
            tracksQueue.pop();

            const size_t trackIndex = cur.first;
            const size_t sampleIndex = cur.second;

            if (sampleIndex + 1 < data.TracksSamples[trackIndex].size()) {
                tracksQueue.push(TIndexPair(trackIndex, sampleIndex + 1));
            }

            const TSampleData& s = data.TracksSamples[trackIndex][sampleIndex];

            TPesPacket pesPacket;
            pesPacket.DataAlignmentIndicator = true;
            pesPacket.Dts = s.Dts.ConvertToTimescale(mpegtsTimescale);
            pesPacket.Cto = s.Cto.ConvertToTimescale(mpegtsTimescale);

            if (DebugTsAsKaltura) {
                // strange offsets from kaltura, only if DebugTsAsKaltura, until we know it is needed
                *pesPacket.Dts += 9090;
            }

            trackPayloadMakers[trackIndex]->MakePayload(s, PayloadTempBuffer);

            pesPacket.StreamIdentifier = track2streamid[trackIndex];

            size_t payloadSize = 0;
            for (const auto& b : PayloadTempBuffer) {
                payloadSize += b.size();
            }

            MakePesPacketHeader(pesPacket, payloadSize, HeaderTempBuffer);
            PayloadTempBuffer.push_front(BlobFromHeaderTempBuffer());
            payloadSize += HeaderTempBuffer.size();

            // now represent this payload by ts packets (work of SpreadPes2Ts)
            const size_t pid = TracksPIDBegin + trackIndex;
            ui32& counter = counters[trackIndex];

            bool firstTsPacket = true;
            while (payloadSize > 0) {
                // -2 bytes for first ts - 1 for adaptation_field_length and 1 for random_access_indicator, (and 6 for Pcr, if trackIndex==0)
                const size_t tsPayloadMaxSize = NMpegts::NTs::PayloadSizeMax - (firstTsPacket ? (trackIndex ? 2 : 8) : 0);

                const size_t curSize = Min(tsPayloadMaxSize, payloadSize);

                if (!firstTsPacket && curSize == defaultTsPayloadSize) {
                    const auto& header = DefaultTsHeaders[trackIndex][counter];
                    AddDataToSend(header.begin(), header.end());
                    counter = (counter + 1) & 0xf;
                } else {
                    TTsPacket tsPacket;
                    tsPacket.PacketIdentifier = pid;
                    tsPacket.ContinuityCounter = counter;
                    counter = (counter + 1) & 0xf;
                    if (firstTsPacket) {
                        tsPacket.PayloadStartIndicator = true;
                        tsPacket.RandomAccessIndicator = s.IsKeyframe();
                        if (trackIndex == 0) {
                            tsPacket.Pcr = s.Dts.ConvertToTimescale(mpegtsPcrTimescale);
                        }

                        if (DebugTsAsKaltura) {
                            // kaltura never set RandomAccessIndicator, it doesnt look good, so do the same only if DebugTsAsKaltura
                            tsPacket.RandomAccessIndicator = false;

                            if (tsPacket.Pcr.Defined()) {
                                // strange offsets from kaltura, only if DebugTsAsKaltura, until we know it is needed
                                *tsPacket.Pcr += 4590 * 300;
                            }
                        }
                    }

                    MakeTsPacketHeader(tsPacket, curSize, /* force_payload_field_flag = */ true, HeaderTempBuffer);

                    AddDataToSend(HeaderTempBuffer);
                }

                // here put in future first curSize bytes from PayloadTempBuffer front
                size_t sizeLeft = curSize;
                while (sizeLeft > 0) {
                    TSimpleBlob& front = PayloadTempBuffer.front();
                    const size_t sizeNow = Min(front.size(), sizeLeft);
                    sizeLeft -= sizeNow;

                    AddDataToSend(front.data(), front.data() + sizeNow);

                    if (sizeNow < front.size()) {
                        front = TSimpleBlob(front.data() + sizeNow, front.size() - sizeNow);
                    } else {
                        PayloadTempBuffer.pop_front();
                    }
                }

                payloadSize -= curSize;
                firstTsPacket = false;

                // optimisation for faster handling of large blobs in PayloadTempBuffer
                if (payloadSize > 0 && PayloadTempBuffer.front().size() >= defaultTsPayloadSize) {
                    Y_ENSURE(!DataToSend.empty());

                    TDataToSendBuffer& toSendBuffer = DataToSend.back();
                    ui8* tsbPos = toSendBuffer.Pos;
                    ui8* const tsbEnd = toSendBuffer.End;

                    ui8 const* pldBegin = PayloadTempBuffer.front().begin();
                    ui8 const* const pldEnd = PayloadTempBuffer.front().end();

                    const auto& tsHeaders = DefaultTsHeaders[trackIndex];

                    while (pldBegin + defaultTsPayloadSize <= pldEnd && tsbPos + defaultTsHeaderSize + defaultTsPayloadSize <= tsbEnd) {
                        auto& tsHeader = tsHeaders[counter];
                        counter = (counter + 1) & 0xf;

                        std::memcpy(tsbPos, tsHeader.data(), tsHeader.size());
                        tsbPos += tsHeader.size();

                        std::memcpy(tsbPos, pldBegin, defaultTsPayloadSize);
                        tsbPos += defaultTsPayloadSize;
                        pldBegin += defaultTsPayloadSize;
                        payloadSize -= defaultTsPayloadSize;
                    }

                    toSendBuffer.Pos = tsbPos;

                    if (pldBegin == pldEnd) {
                        PayloadTempBuffer.pop_front();
                    } else {
                        PayloadTempBuffer.front() = TSimpleBlob(pldBegin, pldEnd);
                    }
                }
            }
        }
    }

    void TMuxerMpegTs::AddPsi(const TVector<TStreamInfo>& streamsInfo) {
        const auto counter = ChunkNumber & 0xf;
        const auto addTsPackets = [this, counter]<typename TContainer>(TContainer&& packets, size_t pid) {
            Y_ENSURE(packets.size() == 1);
            for (TTsPacket& p : packets) {
                p.PacketIdentifier = pid;
                p.ContinuityCounter = counter;

                MakeTsPacketHeader(p, {}, /* force_payload_field_flag = */ true, HeaderTempBuffer);
                AddDataToSend(HeaderTempBuffer);
                AddDataToSend(p.Payload.Gather());
            }
        };

        TSection PatSection;
        TPatSection Pat;
        Pat.AssociationItems.push_back({
            .ProgramNumber = ProgramNumber,
            .ProgramMapPid = ui16(PmtPID),
        });

        PatSection.Body = Pat;

        TSection PmtSection;
        TPmtSection Pmt;
        Pmt.PcrPid = TracksPIDBegin;
        for (size_t i = 0; i < streamsInfo.size(); ++i) {
            const TStreamInfo& si = streamsInfo[i];
            const TMaybe<TBuffer>& pmdescrs = si.ProgramDescriptors;
            const TMaybe<TBuffer>& esdescrs = si.ElementaryStreamDescriptors;

            Pmt.MappingItems.push_back({
                .StreamType = TPmtSection::EStreamType(si.Type),
                .ElementaryPid = ui16(TracksPIDBegin + i),
            });

            if (pmdescrs.Defined()) {
                Pmt.Descriptors.SpliceBack(TBytesChain(TBytesChunk(TBytesContainer(pmdescrs->Data(), pmdescrs->Data() + pmdescrs->Size()))));
            }

            if (esdescrs.Defined()) {
                Pmt.MappingItems.back().Descriptors = TBytesChain(TBytesChunk(TBytesContainer(esdescrs->Data(), esdescrs->Data() + esdescrs->Size())));
            }
        }
        PmtSection.Body = Pmt;

        addTsPackets(NMpegts::NTs::SpreadPsi2Ts(Compose(std::move(PatSection))), PatPID);
        addTsPackets(NMpegts::NTs::SpreadPsi2Ts(Compose(std::move(PmtSection))), PmtPID);

        PsiAdded = true;
    }

    TMuxerMpegTs::TMuxerMpegTs(TRequestWorker& request, ui64 chunkNumber, const TMaybe<TDrmInfo>& drmInfo)
        : Request(request)
        , DataToSendNextBufferSize(16 * 1024)
        , ChunkNumber(chunkNumber)
        , PsiAdded(false)
        , PatPID(0x0)
        , PmtPID(0x1000 + (DebugTsAsKaltura ? -1 : 0))
        , TracksPIDBegin(0x0100)
        , DrmInfo(drmInfo)
    {
        if (DrmInfo.Defined()) {
            Y_ENSURE(DrmInfo->IV.Defined());
            Y_ENSURE(DrmInfo->IV->Size() == DrmKeySize);
            Y_ENSURE(DrmInfo->Key.Size() == DrmIVSize);
        }
    }

    // static
    TMuxerFuture TMuxerMpegTs::Make(TRequestWorker& request, ui64 chunkNumber, const NThreading::TFuture<TMaybe<TDrmInfo>>& drmFuture) {
        // some request to get drm data will be here
        return drmFuture.Apply([&request, chunkNumber](const NThreading::TFuture<TMaybe<TDrmInfo>>& future) -> IMuxer* {
            return request.GetPoolUtil<TMuxerMpegTs>().New(request, chunkNumber, future.GetValue());
        });
    }

    TSimpleBlob TMuxerMpegTs::MakeInitialSegment(const TVector<TTrackInfo const*>& tracksInfo) const {
        (void)tracksInfo;
        ythrow THttpError(NGX_HTTP_NOT_FOUND, TLOG_INFO);
    };

    TRepFuture<TSendData> TMuxerMpegTs::GetData() {
        Y_ENSURE(DataPromise.Initialized());
        return DataPromise.GetFuture();
    }

    void TMuxerMpegTs::SetMediaData(TRepFuture<TMediaData> data) {
        Y_ENSURE(!DataPromise.Initialized());
        DataPromise = TRepPromise<TSendData>::Make();
        RequireData(data, Request).AddCallback(Request.MakeFatalOnException(std::bind(&TMuxerMpegTs::MediaDataCallback, this, std::placeholders::_1)));
    }

    void TMuxerMpegTs::MediaDataCallback(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (dataRef.Empty()) {
            // add some null packets for each track PID, to make zero Continuity counter
            for (size_t i = 0; i < TrackContinuityCounters.size(); ++i) {
                ui32& counter = TrackContinuityCounters[i];

                const auto pid = TracksPIDBegin + i;
                while (counter) {
                    TTsPacket p;
                    p.PacketIdentifier = pid;
                    p.ContinuityCounter = counter;
                    counter = (counter + 1) & 0xf;

                    MakeTsPacketHeader(p, {}, /* force_payload_field_flag = */ true, HeaderTempBuffer);
                    AddDataToSend(HeaderTempBuffer);
                }
            };

            // put data to promise
            for (const TDataToSendBuffer& buf : DataToSend) {
                Y_ENSURE(buf.Pos > buf.Begin);
                DataPromise.PutData(TSendData{TSimpleBlob(buf.Begin, buf.Pos), false});
            }

            // and finish promise
            DataPromise.Finish();

            return;
        }

        TMediaData data = dataRef.Data();
        data.TracksSamples = RemoveOverlappingDts(data.TracksSamples);
        AddFragment(data);
    }

    TSimpleBlob TMuxerMpegTs::BlobFromHeaderTempBuffer() {
        ui8* const data = Request.MemoryPool.AllocateArray<ui8>(HeaderTempBuffer.size(), /*align = */ 1);

        std::memcpy(data, HeaderTempBuffer.data(), HeaderTempBuffer.size());

        return TSimpleBlob{data, HeaderTempBuffer.size()};
    }

    inline void TMuxerMpegTs::AddDataToSend(const TBytesVector& data) {
        ui8 const* const begin = data.data();
        AddDataToSend(begin, begin + data.size());
    }

    inline void TMuxerMpegTs::AddDataToSend(const TBytesChunk& data) {
        ui8 const* const begin = data.View().data();
        AddDataToSend(begin, begin + data.Size());
    }

    inline void TMuxerMpegTs::AddDataToSend(ui8 const* begin, ui8 const* const end) {
        do {
            if (DataToSend.empty() || DataToSend.back().Pos == DataToSend.back().End) {
                TDataToSendBuffer buf;
                buf.Begin = Request.GetPoolUtil<ui8>().Alloc(DataToSendNextBufferSize);
                buf.Pos = buf.Begin;
                buf.End = buf.Begin + DataToSendNextBufferSize;

                DataToSend.push_back(buf);
                DataToSendNextBufferSize *= 2;
            }

            TDataToSendBuffer& buf = DataToSend.back();

            const size_t copySize = Min(end - begin, buf.End - buf.Pos);

            std::memcpy(buf.Pos, begin, copySize);
            buf.Pos += copySize;
            begin += copySize;

        } while (begin < end);
    }

    TString TMuxerMpegTs::GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const {
        return IMuxer::GetContentType(tracksInfo) + "/MP2T";
    }
}
