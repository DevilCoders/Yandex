#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/common/avcC_box_util.h>
#include <nginx/modules/strm_packager/src/common/h2645_util.h>
#include <nginx/modules/strm_packager/src/common/mp4_common.h>
#include <nginx/modules/strm_packager/src/common/muxer_mp4.h>

namespace NStrm::NPackager {
    namespace {
        class TBufferAndBlobsWriter: public NMP4Muxer::TBufferWriter {
        public:
            TBufferAndBlobsWriter()
                : BufferFreeze(false)
                , Position(0)
                , Size(0)
            {
            }

            ui64 GetPosition() const {
                return Position;
            }

            void SetPosition(ui64 newPosition) {
                Position = newPosition;
            }

            template <size_t len, bool prepared = false>
            void WorkIO(void const* buf) {
                if (!prepared) {
                    Y_ENSURE(!BufferFreeze || Position + len <= Buffer().Size());
                }
                NMP4Muxer::TBufferWriter::SetPosition(Position);
                NMP4Muxer::TBufferWriter::WorkIO<len, prepared>(buf);
                Position = Position + len;
                Size = Max(Size, Position);
            }

            template <bool prepared = false>
            void WorkIO(void const* buf, ui64 len) {
                if (!prepared) {
                    Y_ENSURE(!BufferFreeze || Position + len <= Buffer().Size());
                }
                NMP4Muxer::TBufferWriter::SetPosition(Position);
                NMP4Muxer::TBufferWriter::WorkIO<prepared>(buf, len);
                Position = Position + len;
                Size = Max(Size, Position);
            }

            void Prepare(ui64 len) {
                Y_ENSURE(!BufferFreeze);
                NMP4Muxer::TBufferWriter::Prepare(len);
            }

            void WriteBlob(const TSimpleBlob& ref) {
                Y_ENSURE(!ref.empty());
                Y_ENSURE(Position == Size);
                BufferFreeze = true;
                Position = Position + ref.size();
                Size = Position;

                const bool adjacent = !Blobs.empty() && Blobs.back().end() == ref.begin();
                if (adjacent) {
                    Blobs.back() = TSimpleBlob(Blobs.back().begin(), ref.end());
                } else {
                    Blobs.push_back(ref);
                }
            }

            const std::list<TSimpleBlob>& GetBlobs() const {
                return Blobs;
            }

        private:
            bool BufferFreeze;
            ui64 Position;
            ui64 Size;
            std::list<TSimpleBlob> Blobs;
        };

        struct TSample4Mp4Muxer: public TSampleData {
        public:
            TSample4Mp4Muxer(const TSampleData& d)
                : TSampleData(d)
            {
            }

            i64 GetPts() const {
                return TSampleData::GetPts().Value;
            }

            i64 GetDts() const {
                return Dts.Value;
            }
            ui64 GetDuration() const {
                return CoarseDuration.Value;
            }
            ui32 GetDataSize() const {
                return DataSize;
            }
            ui32 GetFlags() const {
                return Flags;
            }

            void Write(TBufferAndBlobsWriter& writer, TBuffer&) const {
                writer.WriteBlob(TSimpleBlob(Data, DataSize));
            }

            void Write(NMP4Muxer::TFakeWriter& writer, TBuffer&) const {
                writer.WorkIO(nullptr, DataSize);
            }
        };

        TVector<TSample4Mp4Muxer> ExtractTimedMetaTrack(
            const TVector<TTrackInfo const*>& tracksInfo,
            TVector<TVector<TSample4Mp4Muxer>>& tracks) {
            // only last track can be timed meta
            for (size_t i = 0; i + 1 < tracksInfo.size(); ++i) {
                Y_ENSURE(!std::holds_alternative<TTrackInfo::TTimedMetaId3Params>(tracksInfo[i]->Params));
            }

            if (std::holds_alternative<TTrackInfo::TTimedMetaId3Params>(tracksInfo.back()->Params)) {
                TVector<TSample4Mp4Muxer> track = std::move(tracks.back());
                tracks.pop_back();
                return std::move(track);
            }

            return {};
        }

        void WriteTimedMetaEmsg(
            TBufferAndBlobsWriter& writer,
            const TVector<TSample4Mp4Muxer>& track) {
            for (const TSample4Mp4Muxer& s : track) {
                Y_ENSURE(s.DataFuture.HasValue());
                Y_ENSURE(s.DataParams.Format == TSampleData::TDataParams::EFormat::TimedMetaId3);

                NMP4Muxer::TDashEventMessageBox emsg;
                emsg.Version = 0;
                emsg.SchemeIdUri = "urn:yandex:meta:id3:2017";
                emsg.Value = "1";
                emsg.Timescale = Ti64TimeP::Timescale;
                emsg.EventDuration = Ms2P(1).Value;
                emsg.Id = 1;
                emsg.PresentationTimeDelta = 0;
                emsg.MessageData.assign(s.Data, s.Data + s.DataSize);

                emsg.WorkIO(writer);
            }
        }
    }

    TMuxerMP4::TMuxerMP4(
        TRequestWorker& request,
        const TMaybe<TDrmInfo>& drmInfo,
        const bool contentLengthPromise,
        const bool addServerTimeUuidBoxes)
        : Request(request)
        , DrmInfo(drmInfo)
        , ProtectionScheme(request.Config.DrmMp4ProtectionScheme.Empty() ? EProtectionScheme::UNSET : request.GetComplexValue<EProtectionScheme>(*request.Config.DrmMp4ProtectionScheme))
        , ContentLengthPromise(contentLengthPromise)
        , AddServerTimeUuidBoxes(addServerTimeUuidBoxes)
        , MoofSequenceNumber(1)
        , PreparedMoofSequenceNumber(1)
        , AllMoofsPrepared(false)
    {
        Y_ENSURE(DrmInfo.Empty() || (ProtectionScheme != EProtectionScheme::UNSET), " DrmInfo.Empty: " << DrmInfo.Empty() << " ProtectionScheme: " << ProtectionScheme);
    }

    // static
    TMuxerFuture TMuxerMP4::Make(
        TRequestWorker& request,
        const NThreading::TFuture<TMaybe<TDrmInfo>>& drmFuture,
        const bool contentLengthPromise,
        const bool addServerTimeUuidBoxes) {
        return drmFuture.Apply([&request, contentLengthPromise, addServerTimeUuidBoxes](const NThreading::TFuture<TMaybe<TDrmInfo>>& future) -> IMuxer* {
            return request.GetPoolUtil<TMuxerMP4>().New(request, future.GetValue(), contentLengthPromise, addServerTimeUuidBoxes);
        });
    }

    // static
    TMuxerMP4::TProtectionData TMuxerMP4::MakeProtectionData(
        const TDrmInfo& drmInfo,
        const EProtectionScheme scheme,
        const bool isNaluVideo) {
        TProtectionData pd;
        pd.Scheme = scheme;

        pd.AuxInfoType = (ui32)scheme;
        pd.AuxInfoTypeParameter = 0;

        switch (scheme) {
            case EProtectionScheme::CENC: // AES-CTR scheme
                pd.ConstantIV = false;
                pd.IVSize = 8;
                pd.BlockSize = 16;
                break;

            case EProtectionScheme::CBC1: // AES-CBC scheme
                pd.ConstantIV = false;
                pd.IVSize = 16;
                pd.BlockSize = 16;
                break;

            case EProtectionScheme::CENS: // AES-CTR subsample pattern encryption scheme
                pd.Pattern.ConstructInPlace();
                pd.Pattern->CryptBlock = 1;
                pd.Pattern->SkipBlock = isNaluVideo ? 9 : 0;
                pd.ConstantIV = false;
                pd.IVSize = 8;
                pd.BlockSize = 16;
                break;

            case EProtectionScheme::CBCS: // AES-CBC subsample pattern encryption scheme
                pd.Pattern.ConstructInPlace();
                pd.Pattern->CryptBlock = 1;
                pd.Pattern->SkipBlock = isNaluVideo ? 9 : 0;
                pd.ConstantIV = true;
                pd.IVSize = 16;
                pd.BlockSize = 16;
                break;

            default:
                Y_ENSURE(false, "unimplemented procetion scheme " << scheme);
        }

        if (drmInfo.IV.Defined()) {
            Y_ENSURE(drmInfo.IV->size() == pd.IVSize);
            pd.IV = drmInfo.IV;
        } else {
            // IV will be generated on demand (different for every fragment)
            // but it is wrong for constant IV
            Y_ENSURE(!pd.ConstantIV);
        }

        return std::move(pd);
    }

    void TMuxerMP4::PrepareProtectionStates(const TVector<TTrackInfo const*>& tracksInfo, const Ti64TimeP beginTs) {
        Y_ENSURE(DrmInfo.Defined());
        Y_ENSURE(TracksProtectionState.empty());

        TracksProtectionState.reserve(tracksInfo.size());

        for (TTrackInfo const* const trackInfo : tracksInfo) {
            TProtectionState state;

            state.TrackInfo = trackInfo;

            const bool isNaluVideo = Mp4TrackParams2TrackDataParams(trackInfo->Params).NalUnitLengthSize.Defined();
            state.Data = MakeProtectionData(*DrmInfo, ProtectionScheme, isNaluVideo);

            if (!state.Data.IV.Defined()) {
                // make iv from beginTs
                state.Data.IV.ConstructInPlace(state.Data.IVSize, '\0');

                for (size_t i = 0; i < state.Data.IVSize; ++i) {
                    (*state.Data.IV)[i] = (ui64(beginTs.Value) >> (56 - 8 * (i % 8))) & 0xff;
                }
            }

            Y_ENSURE(state.Data.IV->length() == state.Data.IVSize);

            // make cipher
            switch (ProtectionScheme) {
                case EProtectionScheme::CENC:
                case EProtectionScheme::CENS:
                    state.Cipher = MakeHolder<TCencCipherAes128Ctr>();
                    break;

                case EProtectionScheme::CBC1:
                case EProtectionScheme::CBCS:
                    state.Cipher = MakeHolder<TCencCipherAes128Cbc>();
                    break;

                default:
                    Y_ENSURE(false, "no cipher for procetion scheme " << ProtectionScheme);
            }

            Y_ENSURE(state.Cipher->IVLength() == state.Data.IVSize);

            Y_ENSURE(state.Cipher->KeyLength() == DrmInfo->Key.length());

            state.Cipher->EncryptInit((ui8 const*)DrmInfo->Key.Data(), (ui8 const*)state.Data.IV->Data());

            TracksProtectionState.push_back(std::move(state));
        }
    }

    void TMuxerMP4::ProtectMoov(NMP4Muxer::TMovieBox& moov, const TVector<TTrackInfo const*>& tracksInfo) const {
        if (DrmInfo.Empty()) {
            return;
        }

        Y_ENSURE(tracksInfo.size() == moov.TrackBoxes.size());

        for (size_t trackIndex = 0; trackIndex < tracksInfo.size(); ++trackIndex) {
            const bool isNaluVideo = Mp4TrackParams2TrackDataParams(tracksInfo[trackIndex]->Params).NalUnitLengthSize.Defined();

            NMP4Muxer::TTrackBox& trackBox = moov.TrackBoxes[trackIndex];
            Y_ENSURE(trackBox.MediaBox.Defined());

            // trak->mdia->minf->stbl->stsd
            NMP4Muxer::TSampleDescriptionBox& stsd = trackBox.MediaBox->MediaInformationBox.SampleTableBox.SampleDescriptionBox;

            NMP4Muxer::TProtectionSchemeInfoBox* sinf;
            ui32 type;

            Y_ENSURE(stsd.VisualSampleEntries.size() + stsd.AudioSampleEntries.size() + stsd.XMLSubtitleSampleEntries.size() == 1);

            if (stsd.VisualSampleEntries.size() > 0) {
                NMP4Muxer::TVisualSampleEntryBox& vb = stsd.VisualSampleEntries[0];
                type = vb.Type;
                vb.Type = 'encv';
                vb.ProtectionSchemeInfoBox.ConstructInPlace();
                sinf = &*vb.ProtectionSchemeInfoBox;
            } else if (stsd.AudioSampleEntries.size() > 0) {
                Y_ENSURE(!isNaluVideo);
                NMP4Muxer::TAudioSampleEntryBox& ab = stsd.AudioSampleEntries[0];
                type = ab.Type;
                ab.Type = 'enca';
                ab.ProtectionSchemeInfoBox.ConstructInPlace();
                sinf = &*ab.ProtectionSchemeInfoBox;
            } else if (stsd.XMLSubtitleSampleEntries.size() > 0) {
                Y_ENSURE(!isNaluVideo);
                NMP4Muxer::TXMLSubtitleSampleEntry& sb = stsd.XMLSubtitleSampleEntries[0];
                type = sb.Type;
                sb.Type = 'encu';
                sb.ProtectionSchemeInfoBox.ConstructInPlace();
                sinf = &*sb.ProtectionSchemeInfoBox;
            } else {
                Y_ENSURE(false, "unsupported track in TMuxerMP4::ProtectMoov");
            }

            sinf->OriginalFormatBox.OriginalFormat = type;

            sinf->SchemeTypeBox.ConstructInPlace();
            sinf->SchemeTypeBox->SchemeType = (ui32)ProtectionScheme;
            sinf->SchemeTypeBox->SchemeVersion = 0x00010000;

            sinf->SchemeInformationBox.ConstructInPlace();

            NMP4Muxer::TTrackEncryptionBox& tenc = sinf->SchemeInformationBox->TrackEncryptionBox;

            Y_ENSURE(DrmInfo->KeyId.length() == sizeof(tenc.DefaultKeyId));
            std::memcpy(tenc.DefaultKeyId, DrmInfo->KeyId.data(), DrmInfo->KeyId.length());

            const TProtectionData pd = MakeProtectionData(*DrmInfo, ProtectionScheme, isNaluVideo);

            if (pd.Pattern.Defined()) {
                tenc.DefaultCryptByteBlock = pd.Pattern->CryptBlock;
                tenc.DefaultSkipByteBlock = pd.Pattern->SkipBlock;
            } else {
                tenc.DefaultCryptByteBlock = 0;
                tenc.DefaultSkipByteBlock = 0;
            }

            tenc.DefaultIsProtected = 1;

            if (pd.ConstantIV) {
                Y_ENSURE(pd.IV.Defined());
                Y_ENSURE(pd.IV->length() == pd.IVSize);
                tenc.DefaultPerSampleIVSize = 0;
                tenc.DefaultConstantIV.assign(pd.IV->data(), pd.IV->data() + pd.IV->length());
            } else {
                tenc.DefaultPerSampleIVSize = pd.IVSize;
            }
        }

        // and add pssh boxes
        for (const TDrmInfo::TPssh& pssh : DrmInfo->Pssh) {
            NMP4Muxer::TProtectionSystemSpecificHeaderBox psshBox;

            Y_ENSURE(pssh.SystemId.length() == sizeof(psshBox.SystemId));
            std::memcpy(psshBox.SystemId, pssh.SystemId.data(), pssh.SystemId.length());

            psshBox.KeyIds.clear(); // just not specified

            psshBox.Data.assign(pssh.Data.data(), pssh.Data.data() + pssh.Data.length());

            moov.ProtectionSystemSpecificHeaderBoxes.push_back(std::move(psshBox));
        }
    }

    TMaybe<ui32> TMuxerMP4::GetSliceDataOffsetAvc(TProtectionState& state, ui8 const* const data, const size_t dataSize) {
        Y_ENSURE(dataSize > 0);

#if 0
        // easy way
        using EAvcNalType = Nh2645::EAvcNalType;
        const ui8 nalTypeByte = data[0];
        const EAvcNalType nalType = EAvcNalType(nalTypeByte & 0x1f);
        if (nalType < EAvcNalType::SLICE || nalType > EAvcNalType::IDR_SLICE) {
            return {};
        } else {
            return 1; // only first byte with nal type, slice header will be encrypted
        }

        (void) state;
#else
        // more complicated way - parse slice header to get it size (no other way)
        if (state.ParsedAvcSps.empty() || state.ParsedAvcPps.empty()) {
            Y_ENSURE(state.TrackInfo);

            const TTrackInfo::TVideoParams& vp = std::get<TTrackInfo::TVideoParams>(state.TrackInfo->Params);
            Y_ENSURE(vp.CodecParamsBox->Type == 'avcC');

            const NAvcCBox::TSpsPpsRange ranges = NAvcCBox::GetSpsPps(vp.CodecParamsBox->Data);

            for (const TSimpleBlob spsRange : ranges.Sps) {
                state.ParsedAvcSps.emplace_back(Nh2645::ParseAvcSps(spsRange));
            }

            for (const TSimpleBlob ppsRange : ranges.Pps) {
                state.ParsedAvcPps.emplace_back(Nh2645::ParseAvcPps(ppsRange, state.ParsedAvcSps));
            }
        }

        return Nh2645::GetSliceDataOffsetAvc(data, dataSize, state.ParsedAvcSps, state.ParsedAvcPps);
#endif
    }

    TMaybe<ui32> TMuxerMP4::GetSliceDataOffsetHevc(TProtectionState& state, ui8 const* const data, const size_t dataSize) {
        Y_ENSURE(dataSize > 0);
#if 0
        // easy way
        (void)state;
        using EHevcNalType = Nh2645::EHevcNalType;
        const ui8 nalTypeByte = data[0];
        const EHevcNalType nalType = EHevcNalType((nalTypeByte >> 1) & 0x3f);
        if (nalType >= EHevcNalType::VPS_NUT) {
            return {};
        } else {
            return 1; // only first byte with nal type, slice header will be encrypted
        }
#else
        if (state.ParsedHevcSps.empty() || state.ParsedHevcPps.empty()) {
            Y_ENSURE(state.TrackInfo);

            const TTrackInfo::TVideoParams& vp = std::get<TTrackInfo::TVideoParams>(state.TrackInfo->Params);
            Y_ENSURE(vp.CodecParamsBox->Type == 'hvcC');

            const NHvcCBox::TParsedHvcC parsed = NHvcCBox::Parse(vp.CodecParamsBox->Data);

            for (const NHvcCBox::TParsedHvcC::TNaluArray& arr : parsed.NaluArrays) {
                for (const TArrayRef<const ui8>& bytes : arr.Nalus) {
                    if (arr.NaluType == (ui32)Nh2645::EHevcNalType::SPS_NUT) {
                        state.ParsedHevcSps.push_back(Nh2645::ParseHevcSps(bytes));
                    } else if (arr.NaluType == (ui32)Nh2645::EHevcNalType::PPS_NUT) {
                        state.ParsedHevcPps.push_back(Nh2645::ParseHevcPps(bytes));
                    }
                }
            }

            Y_ENSURE(!state.ParsedHevcSps.empty());
            Y_ENSURE(!state.ParsedHevcPps.empty());
        }

        return Nh2645::GetSliceDataOffsetHevc(data, dataSize, state.ParsedHevcSps, state.ParsedHevcPps);
#endif
    }

    void TMuxerMP4::ProtectTrack(TProtectionState& state, TSampleData* const trackBegin, TSampleData* const trackEnd, NMP4Muxer::TTrackProtectionAuxInfo& auxInfo) {
        if (trackBegin == trackEnd) {
            return;
        }

        const size_t samplesCount = trackEnd - trackBegin;
        const bool fullSampleEncrypt = trackBegin->DataParams.NalUnitLengthSize.Empty();
        const bool adjustEncryptSizeToBlockSize = ProtectionScheme == EProtectionScheme::CBC1;
        const size_t blockSize = state.Data.BlockSize;

        auxInfo.AuxInfoType = state.Data.AuxInfoType;
        auxInfo.AuxInfoTypeParameter = state.Data.AuxInfoTypeParameter;
        auxInfo.PerSampleIVSize = state.Data.ConstantIV ? 0 : state.Data.IVSize;

        auxInfo.SamplesIV.resize(samplesCount * auxInfo.PerSampleIVSize);

        if (!fullSampleEncrypt) {
            auxInfo.Subsamples.resize(samplesCount);
        }

        for (size_t sampleIndex = 0; sampleIndex < samplesCount; ++sampleIndex, state.Cipher->NextSample()) {
            TSampleData& sample = trackBegin[sampleIndex];

            Y_ENSURE(fullSampleEncrypt == sample.DataParams.NalUnitLengthSize.Empty());

            if (!state.Data.ConstantIV) {
                state.Cipher->GetIV(auxInfo.SamplesIV.data() + sampleIndex * state.Data.IVSize);
            }

            const auto encrypt = [this, &state, blockSize](ui8* output, ui8 const* input, size_t size) {
                if (state.Data.ConstantIV) {
                    // in cbcs mode (constantIV) reinit at the start of each subsample
                    state.Cipher->EncryptInit((ui8 const*)DrmInfo->Key.Data(), (ui8 const*)state.Data.IV->Data());
                }

                if (state.Data.Pattern.Defined()) {
                    TPatternEncryption pattern = *state.Data.Pattern;
                    if (pattern.SkipBlock == 0) {
                        // that mean no pattern, but if last block is partial it must not be encrypted
                        Y_ENSURE(pattern.CryptBlock == 1);
                        // change values so common code below for pattern.SkipBlock > 0 will do what needed
                        pattern.CryptBlock = size / blockSize;
                        pattern.SkipBlock = 1;
                    }

                    while (size > 0) {
                        const size_t encSize = pattern.CryptBlock * blockSize;
                        if (size >= encSize) {
                            state.Cipher->Encrypt(output, input, encSize);
                            size -= encSize;
                            output += encSize;
                            input += encSize;
                        }
                        const size_t skipSize = Min(size, pattern.SkipBlock * blockSize);
                        if (skipSize > 0) {
                            std::memcpy(output, input, skipSize);
                            size -= skipSize;
                            input += skipSize;
                            output += skipSize;
                        }
                    }
                } else {
                    state.Cipher->Encrypt(output, input, size);
                }
            };

            ui8* const encryptedData = Request.MemoryPool.AllocateArray<ui8>(sample.DataSize, /*align = */ 1);

            if (fullSampleEncrypt) {
                encrypt(encryptedData, sample.Data, sample.DataSize);
            } else {
                Y_ENSURE(sample.DataParams.NalUnitLengthSize.Defined());
                const size_t naluLS = *sample.DataParams.NalUnitLengthSize;

                NMP4Muxer::TSampleEncryptionBox::TSubsamplesVector& subsamples = auxInfo.Subsamples[sampleIndex];

                ui8 const* input = sample.Data;
                ui8* output = encryptedData;
                size_t size = sample.DataSize;

                while (size > 0) {
                    Y_ENSURE(size > naluLS);
                    ui32 naluSize = 0;
                    for (size_t i = 0; i < naluLS; ++i) {
                        naluSize = (naluSize << 8) | ui32(input[i]);
                    }
                    Y_ENSURE(naluSize > 0);

                    const size_t fullSize = naluSize + naluLS;
                    Y_ENSURE(size >= fullSize);

                    TMaybe<ui32> sliceDataOffset;
                    switch (sample.DataParams.Format) {
                        case TSampleData::TDataParams::EFormat::VideoAvcc:
                            sliceDataOffset = GetSliceDataOffsetAvc(state, input + naluLS, naluSize);
                            break;

                        case TSampleData::TDataParams::EFormat::VideoHvcc:
                            sliceDataOffset = GetSliceDataOffsetHevc(state, input + naluLS, naluSize);
                            break;

                        default:
                            Y_ENSURE(false, sample.DataParams.Format << " with NalUnitLengthSize set to " << sample.DataParams.NalUnitLengthSize);
                    }

                    const size_t clearOffset = naluLS + sliceDataOffset.GetOrElse(naluSize);

                    size_t encSize = naluSize - (clearOffset - naluLS);
                    if (adjustEncryptSizeToBlockSize) {
                        encSize = (encSize / blockSize) * blockSize;
                    }

                    const size_t tailClearSize = fullSize - clearOffset - encSize;

                    std::memcpy(output, input, clearOffset);
                    subsamples.AddSubsample(clearOffset, /*encrypted =*/false);

                    if (encSize > 0) {
                        encrypt(output + clearOffset, input + clearOffset, encSize);
                        subsamples.AddSubsample(encSize, /*encrypted =*/true);
                    }

                    if (tailClearSize > 0) {
                        std::memcpy(output + clearOffset + encSize, input + clearOffset + encSize, tailClearSize);
                        subsamples.AddSubsample(tailClearSize, /*encrypted =*/false);
                    }

                    input += fullSize;
                    output += fullSize;
                    size -= fullSize;
                }
            }

            sample.Data = encryptedData;
        }
    }

    TSimpleBlob TMuxerMP4::MakeInitialSegment(const TVector<TTrackInfo const*>& tracksInfo) const {
        TVector<NMP4Muxer::TTrackMoovData> moovData(tracksInfo.size());

        for (size_t ti = 0; ti < tracksInfo.size(); ++ti) {
            moovData[ti].AlternateGroup = 0;
            moovData[ti].Timescale = Ti64TimeP::Timescale;
            moovData[ti].Language = tracksInfo[ti]->Language;
            moovData[ti].Name = tracksInfo[ti]->Name;
            moovData[ti].Params = tracksInfo[ti]->Params;

            TTrackInfo::TParams& params = moovData[ti].Params;

            if (auto* vp = std::get_if<TTrackInfo::TVideoParams>(&params)) {
                if (vp->CodecParamsBox->Type == 'avcC') {
                    NAvcCBox::SetNaluSizeLength(vp->CodecParamsBox->Data, ExpectedNaluSizeLength);
                } else if (vp->CodecParamsBox->Type == 'hvcC') {
                    Y_ENSURE(NHvcCBox::GetNaluSizeLength(vp->CodecParamsBox->Data) == ExpectedNaluSizeLength);
                } else if (vp->CodecParamsBox->Type == 'vpcC') {
                    // nothing to check
                } else {
                    Y_ENSURE(false, "unknown video codec params box");
                }
            }
        }

        NMP4Muxer::TBufferWriter writer;

        NMP4Muxer::MakeFileTypeBox().WorkIO(writer);

        NMP4Muxer::TMovieBox moov = NMP4Muxer::MakeMoov(moovData);

        ProtectMoov(moov, tracksInfo);

        moov.WorkIO(writer);

        return Request.HangDataInPool(std::move(writer.Buffer()));
    };

    TRepFuture<TSendData> TMuxerMP4::GetData() {
        Y_ENSURE(DataPromise.Initialized());
        return DataPromise.GetFuture();
    }

    void TMuxerMP4::SetMediaData(TRepFuture<TMediaData> data) {
        Y_ENSURE(!DataPromise.Initialized());
        DataPromise = TRepPromise<TSendData>::Make();

        if (ContentLengthPromise && DrmInfo.Empty()) {
            data.AddCallback(Request.MakeFatalOnException(std::bind(&TMuxerMP4::MediaDataCallbackEmptyMode, this, std::placeholders::_1)));
        }

        RequireData(data, Request).AddCallback(Request.MakeFatalOnException(std::bind(&TMuxerMP4::MediaDataCallback, this, std::placeholders::_1)));
    }

    void TMuxerMP4::MediaDataCallbackEmptyMode(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        Y_ENSURE(ContentLengthPromise);

        // no early moof preparing when drm enabled, since moof will depend on samples data
        Y_ENSURE(!DrmInfo.Defined());

        if (dataRef.Empty()) {
            if (MoofSequenceNumber > 1) {
                // real moof already sent
                return;
            }

            Y_ENSURE(!PreparedMoofs.empty());

            AllMoofsPrepared = true;

            ui64 fullSize = 0;
            for (const TPreparedMoof& pm : PreparedMoofs) {
                fullSize += pm.Moof.size() + pm.MdatSize;
            }

            DataPromise.PutData(TSendData{
                .Blob = PreparedMoofs.front().Moof,
                .Flush = true,
                .ContentLengthPromise = fullSize,
            });

            PreparedMoofs.front().Sent = true;

            return;
        }

        const TMediaData& data = dataRef.Data();

        TVector<TVector<TSample4Mp4Muxer>> tracks = RemoveOverlappingDts<TSample4Mp4Muxer>(data.TracksSamples);

        TBufferAndBlobsWriter writer;
        if (AddServerTimeUuidBoxes) {
            TServerTimeUuidBox(/*with header = */ false).Write(writer);
        }
        WriteTimedMetaEmsg(writer, ExtractTimedMetaTrack(data.TracksInfo, tracks));
        ui64 mdatSize = 0;
        NMP4Muxer::WriteFragment(writer, tracks, PreparedMoofSequenceNumber, /*writeMoof = */ true, /*writeMdat = */ false, &mdatSize);
        Y_ENSURE(writer.GetBlobs().empty());

        PreparedMoofs.push_back(TPreparedMoof{
            .Interval = data.Interval,
            .Moof = Request.HangDataInPool(std::move(writer.Buffer())),
            .MdatSize = mdatSize,
            .Sent = false,
            .MoofSequenceNumber = PreparedMoofSequenceNumber,
        });

        ++PreparedMoofSequenceNumber;
    }

    void TMuxerMP4::MediaDataCallback(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (dataRef.Empty()) {
            DataPromise.Finish();
            return;
        }

        const TMediaData& data = dataRef.Data();

        TVector<TVector<TSample4Mp4Muxer>> tracks = RemoveOverlappingDts<TSample4Mp4Muxer>(data.TracksSamples);

        // esure all data is in correct format
        for (size_t trackIndex = 0; trackIndex < tracks.size(); ++trackIndex) {
            TSampleData::TDataParams expectedDataParams = Mp4TrackParams2TrackDataParams(data.TracksInfo[trackIndex]->Params);
            if (expectedDataParams.NalUnitLengthSize.Defined()) {
                expectedDataParams.NalUnitLengthSize = ExpectedNaluSizeLength;
            }

            if (std::holds_alternative<TTrackInfo::TSubtitleParams>(data.TracksInfo[trackIndex]->Params)) {
                Y_ENSURE(expectedDataParams.Format == TSampleData::TDataParams::EFormat::SubtitleTTML);
            }

            for (const TSampleData& s : tracks[trackIndex]) {
                // TODO: convert if params differ
                Y_ENSURE(s.DataParams == expectedDataParams);
            }
        }

        if (AllMoofsPrepared) {
            Y_ENSURE(DrmInfo.Empty());
            Y_ENSURE(!PreparedMoofs.empty());
            const TPreparedMoof& pm = PreparedMoofs.front();

            Y_ENSURE(pm.Interval.Begin == data.Interval.Begin);
            Y_ENSURE(pm.Interval.End == data.Interval.End);
            Y_ENSURE(pm.MoofSequenceNumber == MoofSequenceNumber);
            Y_ENSURE(pm.Sent == (MoofSequenceNumber == 1));

            ExtractTimedMetaTrack(data.TracksInfo, tracks);

            TBufferAndBlobsWriter writer;
            ui64 mdatSize = 0;
            NMP4Muxer::WriteFragment(writer, tracks, MoofSequenceNumber, /*writeMoof = */ false, /*writeMdat = */ true, &mdatSize);
            ++MoofSequenceNumber;

            Y_ENSURE(pm.MdatSize == writer.GetPosition());

            if (!pm.Sent) {
                DataPromise.PutData(TSendData{pm.Moof, false});
            }
            PreparedMoofs.pop_front();

            DataPromise.PutData(TSendData{Request.HangDataInPool(std::move(writer.Buffer())), false});
            for (const TSimpleBlob& blob : writer.GetBlobs()) {
                DataPromise.PutData(TSendData{blob, false});
            }
            DataPromise.PutData(TSendData{.Flush = true});

        } else {
            TMaybe<TVector<NMP4Muxer::TTrackProtectionAuxInfo>> ProtectionAuxInfo;
            if (DrmInfo.Defined()) {
                if (TracksProtectionState.empty()) {
                    PrepareProtectionStates(data.TracksInfo, data.Interval.Begin);
                }

                Y_ENSURE(TracksProtectionState.size() == tracks.size());

                ProtectionAuxInfo.ConstructInPlace(tracks.size());

                for (size_t i = 0; i < tracks.size(); ++i) {
                    ProtectTrack(TracksProtectionState[i], tracks[i].data(), tracks[i].data() + tracks[i].size(), ProtectionAuxInfo->at(i));
                }
            }

            TBufferAndBlobsWriter writer;
            if (AddServerTimeUuidBoxes) {
                TServerTimeUuidBox(/*with header = */ false).Write(writer);
            }
            WriteTimedMetaEmsg(writer, ExtractTimedMetaTrack(data.TracksInfo, tracks));
            NMP4Muxer::WriteFragment(writer, tracks, MoofSequenceNumber, /*writeMoof = */ true, /*writeMdat = */ true, /*mdatSize = */ nullptr, std::move(ProtectionAuxInfo));
            ++MoofSequenceNumber;

            DataPromise.PutData(TSendData{Request.HangDataInPool(std::move(writer.Buffer())), false});
            for (const TSimpleBlob& blob : writer.GetBlobs()) {
                DataPromise.PutData(TSendData{blob, false});
            }
            DataPromise.PutData(TSendData{.Flush = true});
        }
    }

    TString TMuxerMP4::GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const {
        const TString tracksType = IMuxer::GetContentType(tracksInfo);
        if (tracksType == "text") {
            return "application/mp4";
        }
        return tracksType + "/mp4";
    }
}
