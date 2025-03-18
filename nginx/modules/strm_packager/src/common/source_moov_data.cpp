#include <nginx/modules/strm_packager/src/common/mp4_common.h>
#include <nginx/modules/strm_packager/src/common/source.h>

#include <strm/media/transcoder/mp4muxer/io.h>

namespace NStrm::NPackager {
    namespace {
        using TAudioParams = TTrackInfo::TAudioParams;
        using TVideoParams = TTrackInfo::TVideoParams;
        using TSubtitleParams = TTrackInfo::TSubtitleParams;
        using TParams = TTrackInfo::TParams;

        template <typename TIOWorker, typename T, typename... Args>
        void VectorWorkIO(TIOWorker& ioworker, TVector<T>& data, Args&... args);

        template <typename TIOWorker, typename TBoxType>
        inline decltype(std::declval<TBoxType&>().WorkIO(std::declval<TIOWorker&>())) WorkIO(TIOWorker& ioworker, TBoxType& someBox) {
            someBox.template WorkIO<TIOWorker>(ioworker);
        }

        template <typename TIOWorker, typename T>
        inline decltype(EasyWorkIO(std::declval<TIOWorker&>(), std::declval<T&>())) WorkIO(TIOWorker& ioworker, T& data) {
            EasyWorkIO(ioworker, data);
        }

        template <typename TIOWorker, typename T>
        inline void WorkIO(TIOWorker& ioworker, TMaybe<T>& mb) {
            ui8 defined = mb.Defined();
            EasyWorkIO(ioworker, defined);
            if (defined) {
                if (!mb.Defined()) {
                    mb.ConstructInPlace();
                }
                WorkIO(ioworker, *mb);
            } else {
                mb.Clear();
            }
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, ui32& v) {
            EasyWorkIO(ioworker, v);
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, TBuffer& buffer) {
            size_t size = buffer.Size();
            EasyWorkIO(ioworker, size);
            buffer.Resize(size);
            ioworker.WorkIO(buffer.Data(), size);
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, TTrackInfo::TTimedMetaId3Params& params) {
            (void)ioworker;
            (void)params;
            Y_ENSURE(false, "called WorkIO for TTimedMetaId3Params");
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, TVideoParams& params) {
            EasyWorkIO(ioworker, params.Width);
            EasyWorkIO(ioworker, params.Height);
            WorkIO(ioworker, params.PixelAspectRatioWidth);
            WorkIO(ioworker, params.PixelAspectRatioHeight);
            EasyWorkIO(ioworker, params.BoxType);
            WorkIO(ioworker, params.CodecParamsBox);
            VectorWorkIO(ioworker, params.OtherBoxes);
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, TAudioParams& params) {
            EasyWorkIO(ioworker, params.ChannelCount);
            EasyWorkIO(ioworker, params.SampleRate);
            EasyWorkIO(ioworker, params.BoxType);
            EasyWorkIO(ioworker, params.CodecType);
            WorkIO(ioworker, params.EsdsTrackIdPosition);
            WorkIO(ioworker, params.DecoderSpecificInfo);
            WorkIO(ioworker, params.ESDescriptorBox);
            WorkIO(ioworker, params.AC3SpecificBox);
            WorkIO(ioworker, params.EC3SpecificBox);
            WorkIO(ioworker, params.OpusSpecificBox);
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, TSubtitleParams& params) {
            (void)ioworker;
            (void)params;
            Y_ENSURE(false, "TSubtitleParams must no be written in moov cache");
        }

        template <class T, size_t I = 0>
        void EmplaceVariant(T& variant, size_t i) {
            if (i == I) {
                variant.template emplace<I>();
            } else if constexpr (I + 1 < std::variant_size_v<T>) {
                EmplaceVariant<T, I + 1>(variant, i);
            } else {
                ythrow yexception() << "WorkIO: unknown track type";
            }
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, TParams& params) {
            size_t index = params.index();
            EasyWorkIO(ioworker, index);

            if (index != params.index()) {
                EmplaceVariant(params, index);
            }

            std::visit([&](auto& x) { WorkIO(ioworker, x); }, params);
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, TTrackInfo& info) {
            EasyWorkIO(ioworker, info.Language);
            StringWorkIO(ioworker, info.Name);
            WorkIO(ioworker, info.Params);
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, TFileMediaData::TFileSample& sample) {
            EasyWorkIO(ioworker, sample.Dts);
            EasyWorkIO(ioworker, sample.Cto);
            EasyWorkIO(ioworker, sample.CoarseDuration);
            EasyWorkIO(ioworker, sample.Flags);
            EasyWorkIO(ioworker, sample.DataSize);
            EasyWorkIO(ioworker, sample.DataFileOffset);

            if (ioworker.IsReader()) {
                sample.DataBlob = nullptr;
                sample.DataBlobOffset = 0;
            }
        }

        template <typename TIOWorker>
        inline void WorkIO(TIOWorker& ioworker, TSampleData::TDataParams& dp) {
            EasyWorkIO(ioworker, dp.Format);
            WorkIO(ioworker, dp.NalUnitLengthSize);
        }

        template <typename TReader, typename T, ui64 stride>
        class TReaderIterator: public std::iterator<
                                   /* iterator_category */ std::random_access_iterator_tag,
                                   /* value_type        */ const T,
                                   /* difference_type   */ ui64,
                                   /* pointer           */ const T,
                                   /* reference         */ const T> {
        public:
            static_assert(TReader::IsReader());

            TReaderIterator(TReader* reader, ui64 position)
                : Reader(reader)
                , Position(position)
            {
            }

            void SetPosition() const {
                Reader->SetPosition(Position);
            }

            TReaderIterator& operator+=(const i64 x) {
                Position += x * stride;
                return *this;
            }

            TReaderIterator operator++() {
                Position += stride;
                return *this;
            }

            T operator*() const {
                SetPosition();
                T value = 0;
                EasyWorkIO(*Reader, value);
                return value;
            }

            friend inline ui64 operator-(const TReaderIterator& a, const TReaderIterator& b) {
                return (a.Position - b.Position) / stride;
            }

            friend inline bool operator<(const TReaderIterator& a, const TReaderIterator& b) {
                return a.Position < b.Position;
            }

            friend inline bool operator==(const TReaderIterator& a, const TReaderIterator& b) {
                return a.Position == b.Position;
            }

        public:
            TReader* Reader;
            ui64 Position;
        };

        // only write, actually
        template <typename TIOWriter>
        inline void WorkIO(TIOWriter& writer, const TMoovData::TTrackSamples& samples) {
            static_assert(TIOWriter::IsWriter());

            Y_ENSURE(samples.SamplesCount == 0 || samples.DtsBlocks.size() > 0);
            Y_ENSURE(samples.Cto.Empty() || samples.Cto->size() == samples.SamplesCount);
            Y_ENSURE(samples.DataSize.size() == samples.SamplesCount);
            Y_ENSURE(samples.SamplesCount == 0 || samples.DataBlocks.size() > 0);

            EasyWorkIO(writer, samples.SamplesCount);
            EasyWorkIO(writer, samples.Timescale);
            EasyWorkIO(writer, samples.MinCto);
            EasyWorkIO(writer, samples.MaxCto);

            const ui32 dtsblocksCount = samples.DtsBlocks.size();
            const ui8 ctoDefined = samples.Cto.Defined();
            const ui32 keyframesCount = samples.Keyframes.size();
            const ui32 datablocksCount = samples.DataBlocks.size();
            EasyWorkIO(writer, dtsblocksCount);
            EasyWorkIO(writer, ctoDefined);
            EasyWorkIO(writer, keyframesCount);
            EasyWorkIO(writer, datablocksCount);

            if (samples.SamplesCount == 0) {
                return;
            }

            for (const TMoovData::TTrackSamples::TDtsBlock& db : samples.DtsBlocks) {
                EasyWorkIO(writer, db.DtsBegin);
                EasyWorkIO(writer, db.Duration);
                EasyWorkIO(writer, db.IndexBegin);
            }

            if (ctoDefined) {
                for (const i32& cto : *samples.Cto) {
                    EasyWorkIO(writer, cto);
                }
            }

            for (const ui32& kf : samples.Keyframes) {
                EasyWorkIO(writer, kf);
            }

            for (const ui32& ds : samples.DataSize) {
                EasyWorkIO(writer, ds);
            }

            for (const TMoovData::TTrackSamples::TDataBlock& db : samples.DataBlocks) {
                EasyWorkIO(writer, db.IndexBegin);
                EasyWorkIO(writer, db.DataBeginOffset);
            }
        }

        // only write, actually
        //   alternative version of saving TTrackSamples, when sample tables are read directly from moov box buffer
        template <typename TIOWriter>
        inline void WorkIO(TIOWriter& writer, const TMoovData::TTrackSamples& samples, NMP4Muxer::TBlobReader& reader, NMP4Muxer::TMovieBox& moov) {
            static_assert(TIOWriter::IsWriter());

            const i64 dtsOffsetByEditListBox = samples.DtsOffsetByEditListBox;

            NMP4Muxer::TTrackBox const* trackBoxPtr = nullptr;
            for (const auto& tb : moov.TrackBoxes) {
                if (tb.TrackHeaderBox->TrackID == samples.TrackID) {
                    Y_ENSURE(!trackBoxPtr);
                    trackBoxPtr = &tb;
                }
            }
            Y_ENSURE(trackBoxPtr);
            const NMP4Muxer::TSampleTableBox& stbl = trackBoxPtr->MediaBox->MediaInformationBox.SampleTableBox;

            const ui32 samplesCount = stbl.SampleSizeBox.Defined() ? stbl.SampleSizeBox->SamplesCount : stbl.CompactSampleSizeBox->SamplesCount;
            const ui32 timescale = trackBoxPtr->MediaBox->MediaHeaderBox.Timescale;

            i32 minCto = 0;
            i32 maxCto = 0;
            ui32 dtsblocksCount = 0;
            ui8 ctoDefined = 0;
            ui32 keyframesCount = 0;
            ui32 datablocksCount = 0;

            EasyWorkIO(writer, samplesCount);
            EasyWorkIO(writer, timescale);

            const ui64 minCtoPosition = writer.GetPosition();
            EasyWorkIO(writer, minCto);
            EasyWorkIO(writer, maxCto);
            EasyWorkIO(writer, dtsblocksCount);
            EasyWorkIO(writer, ctoDefined);
            EasyWorkIO(writer, keyframesCount);
            EasyWorkIO(writer, datablocksCount);

            if (samplesCount == 0) {
                return;
            }

            // dts blocks
            {
                // 1 dts block = 1 stts entry
                const NMP4Muxer::TTimeToSampleBox& stts = *stbl.TimeToSampleBox;
                Y_ENSURE(stts.Entries.empty());

                dtsblocksCount = stts.EntriesCount;
                ui32 indexBegin = 0;
                i64 dtsBegin = dtsOffsetByEditListBox;

                reader.SetPosition(stts.EntriesBeginPosition);
                for (ui32 blockIndex = 0; blockIndex < dtsblocksCount; ++blockIndex) {
                    NMP4Muxer::TTimeToSampleBox::TEntry entry;
                    entry.WorkIO(reader);

                    const ui32 count = entry.SampleCount;
                    const ui32 duration = entry.SampleDelta;

                    EasyWorkIO(writer, dtsBegin);
                    EasyWorkIO(writer, duration);
                    EasyWorkIO(writer, indexBegin);

                    indexBegin += count;
                    dtsBegin += i64(duration) * i64(count);
                }
                Y_ENSURE(indexBegin == samplesCount);
            }

            // cto
            ctoDefined = stbl.CompositionTimeToSampleBox.Defined();
            if (ctoDefined) {
                const NMP4Muxer::TCompositionTimeToSampleBox& ctts = *stbl.CompositionTimeToSampleBox;
                Y_ENSURE(ctts.Entries.empty());

                ui32 countAcc = 0;

                reader.SetPosition(ctts.EntriesBeginPosition);

                reader.Prepare(ctts.EntriesCount * NMP4Muxer::TCompositionTimeToSampleBox::TEntry::BytesSize);
                writer.Prepare(samplesCount * 4);

                ui32 entryIndex = 0;

                static const int N = 4;
                for (; entryIndex + N - 1 < ctts.EntriesCount; entryIndex += N) {
                    NMP4Muxer::TCompositionTimeToSampleBox::TEntry entry[N];
                    for (int j = 0; j < N; ++j) {
                        entry[j].WorkIO<NMP4Muxer::TBlobReader, true>(reader, ctts.Version);
                    }

                    for (int j = 0; j < N; ++j) {
                        const ui32 count = entry[j].SampleCount;
                        const i32 cto = entry[j].SampleOffset;

                        minCto = Min(minCto, cto);
                        maxCto = Max(maxCto, cto);

                        if (count == 1) {
                            ++countAcc;
                            EasyWorkIO<TIOWriter, const i32, true>(writer, cto);
                        } else if (count == 2) {
                            countAcc += 2;
                            EasyWorkIO<TIOWriter, const i32, true>(writer, cto);
                            EasyWorkIO<TIOWriter, const i32, true>(writer, cto);
                        } else {
                            countAcc += count;
                            for (ui32 k = 0; k < count; ++k) {
                                EasyWorkIO<TIOWriter, const i32, true>(writer, cto);
                            }
                        }
                    }
                }

                for (; entryIndex < ctts.EntriesCount; ++entryIndex) {
                    NMP4Muxer::TCompositionTimeToSampleBox::TEntry entry;
                    entry.WorkIO<NMP4Muxer::TBlobReader, true>(reader, ctts.Version);

                    const ui32 count = entry.SampleCount;
                    const i32 cto = entry.SampleOffset;

                    minCto = Min(minCto, cto);
                    maxCto = Max(maxCto, cto);

                    if (count == 1) {
                        ++countAcc;
                        EasyWorkIO<TIOWriter, const i32, true>(writer, cto);
                    } else if (count == 2) {
                        countAcc += 2;
                        EasyWorkIO<TIOWriter, const i32, true>(writer, cto);
                        EasyWorkIO<TIOWriter, const i32, true>(writer, cto);
                    } else {
                        countAcc += count;
                        for (ui32 k = 0; k < count; ++k) {
                            EasyWorkIO<TIOWriter, const i32, true>(writer, cto);
                        }
                    }
                }
                Y_ENSURE(countAcc == samplesCount);
            }

            // keyframes
            if (stbl.SyncSampleBox.Defined()) {
                const NMP4Muxer::TSyncSampleBox& stss = *stbl.SyncSampleBox;
                Y_ENSURE(stss.SampleNumber.empty());

                keyframesCount = stss.NumbersCount;

                reader.SetPosition(stss.NumbersBeginPosition);
                for (ui32 numIndex = 0; numIndex < keyframesCount; ++numIndex) {
                    const ui32 index = ReadUi32BigEndian(reader) - 1;
                    Y_ENSURE(index < samplesCount);
                    EasyWorkIO(writer, index);
                }
            }

            // data size and data blocks
            {
                // prepare chunk offset read
                ui64 chunksOffsetReadPosition;
                bool largeChanksOffsets;
                ui32 chunksCount_;
                if (stbl.ChunkOffsetBox.Defined()) {
                    Y_ENSURE(stbl.ChunkOffsetBox->ChunkOffsets.empty());
                    largeChanksOffsets = false;
                    chunksOffsetReadPosition = stbl.ChunkOffsetBox->OffsetsBeginPosition;
                    chunksCount_ = stbl.ChunkOffsetBox->ChunksCount;
                } else {
                    Y_ENSURE(stbl.ChunkLargeOffsetBox->ChunkOffsets.empty());
                    largeChanksOffsets = true;
                    chunksOffsetReadPosition = stbl.ChunkLargeOffsetBox->OffsetsBeginPosition;
                    chunksCount_ = stbl.ChunkLargeOffsetBox->ChunksCount;
                }
                const ui32 chunksCount = chunksCount_;

                reader.SetPosition(chunksOffsetReadPosition);
                reader.Prepare(chunksCount * (largeChanksOffsets ? 8 : 4));

                // prepare sample data size read
                ui32 sizeType;
                ui32 sizeType0AllSamplesSize;
                ui64 sampleDataSizeReadingPosition = 0;
                ui64 dataSizeWritePosition = writer.GetPosition();
                writer.Prepare(samplesCount * 4);

                if (stbl.SampleSizeBox.Defined()) {
                    const NMP4Muxer::TSampleSizeBox& stsz = *stbl.SampleSizeBox;
                    Y_ENSURE(stsz.SamplesSize.empty());
                    Y_ENSURE(samplesCount == stsz.SamplesCount);

                    if (stsz.AllSamplesSize != 0) {
                        sizeType = 0; // all samples have equal size
                        sizeType0AllSamplesSize = stsz.AllSamplesSize;
                    } else {
                        sampleDataSizeReadingPosition = stsz.SizesBeginPosition;
                        sizeType = 1; // 32-bit size
                    }
                } else {
                    Y_ENSURE(stbl.CompactSampleSizeBox.Defined());
                    // here can be
                    const NMP4Muxer::TCompactSampleSizeBox& stz2 = *stbl.CompactSampleSizeBox;
                    Y_ENSURE(stz2.SamplesSize.empty());
                    Y_ENSURE(samplesCount == stz2.SamplesCount);
                    sampleDataSizeReadingPosition = stz2.SizesBeginPosition;

                    if (stz2.FieldSize == 16) {
                        sizeType = 2; // 16-bit size
                    } else if (stz2.FieldSize == 8) {
                        sizeType = 3; // 8-bit size
                    } else if (stz2.FieldSize == 4) {
                        sizeType = 3; // 4-bit size
                        Y_ENSURE(false, "4-bit size is not implemented");
                    } else {
                        Y_ENSURE(false, "TCompactSampleSizeBox unexpected filed size " << stz2.FieldSize);
                    }
                }

                // prepare sample to chunk box read
                const NMP4Muxer::TSampleToChunkBox& stsc = *stbl.SampleToChunkBox;
                Y_ENSURE(stsc.Entries.empty());
                ui32 stscEntriesCounter = stsc.EntriesCount;
                ui64 stscEntriesPosition = stsc.EntriesBeginPosition;
                const NMP4Muxer::TSampleToChunkBox::TEntry stscEndEntry = {
                    .FirstChunk = chunksCount + 1111, // just large enough index so it will be unreachable in the code below
                    .SamplesPerChunk = 0,
                    .SampleDescriptionIndex = 1,
                };
                reader.SetPosition(stscEntriesPosition);
                reader.Prepare(stsc.EntriesCount * NMP4Muxer::TSampleToChunkBox::TEntry::BytesSize);

                NMP4Muxer::TSampleToChunkBox::TEntry stscCurEntry = stscEndEntry;
                NMP4Muxer::TSampleToChunkBox::TEntry stscNextEntry = stscEndEntry;
                if (stscEntriesCounter > 0) {
                    --stscEntriesCounter;
                    reader.SetPosition(stscEntriesPosition);
                    stscCurEntry.WorkIO<NMP4Muxer::TBlobReader, true>(reader);
                    stscEntriesPosition = reader.GetPosition();
                    if (stscEntriesCounter > 0) {
                        --stscEntriesCounter;
                        stscNextEntry.WorkIO<NMP4Muxer::TBlobReader, true>(reader);
                        stscEntriesPosition = reader.GetPosition();
                    }
                }
                Y_ENSURE(stscCurEntry.FirstChunk == 1);

                // start work with sample size and chunks
                ui64 dataBlocksWritePosition = dataSizeWritePosition + samplesCount * 4;
                ui32 sampleIndex = 0;
                for (ui32 chunkIndex = 0; chunkIndex < chunksCount; ++chunkIndex) {
                    ui64 dataOffset;
                    {
                        reader.SetPosition(chunksOffsetReadPosition);
                        dataOffset = largeChanksOffsets ? ReadUi64BigEndian<NMP4Muxer::TBlobReader, true>(reader) : ReadUi32BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                        chunksOffsetReadPosition = reader.GetPosition();
                    }

                    if (chunkIndex + 1 == stscNextEntry.FirstChunk) {
                        stscCurEntry = stscNextEntry;
                        stscNextEntry = stscEndEntry;
                        if (stscEntriesCounter > 0) {
                            --stscEntriesCounter;
                            reader.SetPosition(stscEntriesPosition);
                            stscNextEntry.WorkIO<NMP4Muxer::TBlobReader, true>(reader);
                            stscEntriesPosition = reader.GetPosition();
                        }
                    }

                    ui32 samplesInChunk = stscCurEntry.SamplesPerChunk;

                    while (samplesInChunk > 0) {
                        const ui32 samplesInBlock = Min(samplesInChunk, TMoovData::TTrackSamples::DataBlockMaxSamplesCount);

                        // write block begining
                        ++datablocksCount;
                        ui64 prevWriterPosition = writer.GetPosition();
                        writer.SetPosition(dataBlocksWritePosition);
                        writer.Prepare(12);
                        EasyWorkIO<TIOWriter, const ui32, true>(writer, sampleIndex); // index begin
                        EasyWorkIO<TIOWriter, const ui64, true>(writer, dataOffset);
                        dataBlocksWritePosition = writer.GetPosition();
                        writer.SetPosition(prevWriterPosition);

                        reader.SetPosition(sampleDataSizeReadingPosition);
                        Y_ENSURE(dataSizeWritePosition == writer.GetPosition());

                        if (sizeType == 0) {
                            for (ui32 si = 0; si < samplesInBlock; ++si) {
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, sizeType0AllSamplesSize);
                                dataOffset += sizeType0AllSamplesSize;
                            }
                        } else if (sizeType == 1) { // 32-bit size
                            // read data size, write data size, update data offset
                            ui32 si = 0;
                            for (; si + 3 < samplesInBlock; si += 4) {
                                const ui32 dataSize0 = ReadUi32BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                const ui32 dataSize1 = ReadUi32BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                const ui32 dataSize2 = ReadUi32BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                const ui32 dataSize3 = ReadUi32BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize0);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize1);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize2);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize3);
                                dataOffset += dataSize0 + dataSize1 + dataSize2 + dataSize3;
                            }

                            for (; si < samplesInBlock; ++si) {
                                const ui32 dataSize = ReadUi32BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize);
                                dataOffset += dataSize;
                            }
                        } else if (sizeType == 2) { // 16-bit size
                            // read data size, write data size, update data offset
                            ui32 si = 0;
                            for (; si + 3 < samplesInBlock; si += 4) {
                                const ui32 dataSize0 = ReadUi16BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                const ui32 dataSize1 = ReadUi16BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                const ui32 dataSize2 = ReadUi16BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                const ui32 dataSize3 = ReadUi16BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize0);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize1);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize2);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize3);
                                dataOffset += dataSize0 + dataSize1 + dataSize2 + dataSize3;
                            }

                            for (; si < samplesInBlock; ++si) {
                                const ui32 dataSize = ReadUi16BigEndian<NMP4Muxer::TBlobReader, true>(reader);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize);
                                dataOffset += dataSize;
                            }
                        } else if (sizeType == 3) { // 8-bit size
                            // read data size, write data size, update data offset
                            ui32 si = 0;
                            for (; si + 3 < samplesInBlock; si += 4) {
                                const ui32 dataSize0 = ReadUI8<NMP4Muxer::TBlobReader, true>(reader);
                                const ui32 dataSize1 = ReadUI8<NMP4Muxer::TBlobReader, true>(reader);
                                const ui32 dataSize2 = ReadUI8<NMP4Muxer::TBlobReader, true>(reader);
                                const ui32 dataSize3 = ReadUI8<NMP4Muxer::TBlobReader, true>(reader);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize0);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize1);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize2);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize3);
                                dataOffset += dataSize0 + dataSize1 + dataSize2 + dataSize3;
                            }

                            for (; si < samplesInBlock; ++si) {
                                const ui32 dataSize = ReadUI8<NMP4Muxer::TBlobReader, true>(reader);
                                EasyWorkIO<TIOWriter, const ui32, true>(writer, dataSize);
                                dataOffset += dataSize;
                            }
                        }
                        dataSizeWritePosition = writer.GetPosition();
                        sampleDataSizeReadingPosition = reader.GetPosition();

                        samplesInChunk -= samplesInBlock;
                        sampleIndex += samplesInBlock;
                    }
                }

                Y_ENSURE(stscEntriesCounter == 0);
                Y_ENSURE(sampleIndex == samplesCount);

                writer.SetPosition(dataBlocksWritePosition);
            }

            const ui64 endPosition = writer.GetPosition();
            writer.SetPosition(minCtoPosition);
            EasyWorkIO(writer, minCto);
            EasyWorkIO(writer, maxCto);
            EasyWorkIO(writer, dtsblocksCount);
            EasyWorkIO(writer, ctoDefined);
            EasyWorkIO(writer, keyframesCount);
            EasyWorkIO(writer, datablocksCount);
            writer.SetPosition(endPosition);
        }

        // read TMoovData::TTrackSamples data in TVector<TFileMediaData::TFileSample>
        //   data in reader must be created by `void WorkIO(TIOWriter& writer, const TMoovData::TTrackSamples& samples)`
        template <typename TIOReader>
        inline void WorkIO(TIOReader& reader, TVector<TFileMediaData::TFileSample>& samples, const TMaybe<TIntervalP>& interval) {
            static_assert(TIOReader::IsReader());

            static const int dtsBlockSize = 16;
            static const int ctoSize = 4;
            static const int keyframeSize = 4;
            static const int datasizeSize = 4;
            static const int datablockSize = 12;

            using TDtsBlock = TMoovData::TTrackSamples::TDtsBlock;
            using TDataBlock = TMoovData::TTrackSamples::TDataBlock;

            using TDtsIterator = TReaderIterator<TIOReader, i64, dtsBlockSize>;       // iterator to access DtsBegin field of saved TDtsBlock
            using TKeyframeIterator = TReaderIterator<TIOReader, ui32, keyframeSize>; // iterator to access keyframe index
            using TIdxIterator = TReaderIterator<TIOReader, ui32, datablockSize>;     // iterator to access IndexBegin field of saved TDataBlock

            ui32 samplesCount = 0;
            ui32 timescale = 0;
            i32 minCto = 0;
            i32 maxCto = 0;
            ui32 dtsblocksCount = 0;
            ui8 ctoDefined = 0;
            ui32 keyframesCount = 0;
            ui32 datablocksCount = 0;

            EasyWorkIO(reader, samplesCount);
            EasyWorkIO(reader, timescale);
            EasyWorkIO(reader, minCto);
            EasyWorkIO(reader, maxCto);
            EasyWorkIO(reader, dtsblocksCount);
            EasyWorkIO(reader, ctoDefined);
            EasyWorkIO(reader, keyframesCount);
            EasyWorkIO(reader, datablocksCount);

            if (samplesCount == 0) {
                return;
            }

            const ui64 dtsBlocksBegin = reader.GetPosition();
            const ui64 dtsBlocksEnd = dtsBlocksBegin + dtsblocksCount * dtsBlockSize;

            const ui64 ctoBegin = dtsBlocksEnd;
            const ui64 ctoEnd = ctoBegin + (ctoDefined ? samplesCount * ctoSize : 0);

            const ui64 keyframesBegin = ctoEnd;
            const ui64 keyframesEnd = keyframesBegin + keyframesCount * keyframeSize;

            const ui64 dataSizeBegin = keyframesEnd;
            const ui64 dataSizeEnd = dataSizeBegin + samplesCount * datasizeSize;

            const ui64 dataBlocksBegin = dataSizeEnd;
            const ui64 dataBlocksEnd = dataBlocksBegin + datablocksCount * datablockSize;

            const ui64 theEnd = dataBlocksEnd;

            const TDtsIterator allBLocksBegin = TDtsIterator(&reader, dtsBlocksBegin);
            const TDtsIterator allBLocksEnd = TDtsIterator(&reader, dtsBlocksEnd);

            const auto readDtsBlock = [&reader](const TDtsIterator& it) -> TDtsBlock {
                it.SetPosition();
                TDtsBlock result;
                EasyWorkIO(reader, result.DtsBegin);
                EasyWorkIO(reader, result.Duration);
                EasyWorkIO(reader, result.IndexBegin);
                return result;
            };

            const auto readDataBlock = [&reader](const TIdxIterator& it) -> TDataBlock {
                it.SetPosition();
                TDataBlock result;
                EasyWorkIO(reader, result.IndexBegin);
                EasyWorkIO(reader, result.DataBeginOffset);
                return result;
            };

            const auto convertToP = [timescale](const i64 ts) -> i64 {
                if (timescale == Ti64TimeP::Timescale) {
                    return ts;
                } else {
                    return ConvertTimescaleFloor(ts, timescale, Ti64TimeP::Timescale);
                }
            };

            // determine index range

            ui32 indexBegin = 0;
            ui32 indexEnd = samplesCount;

            TDtsIterator blocksBegin = allBLocksBegin;

            if (interval.Defined() && interval->Begin == interval->End) {
                indexEnd = 0;
            } else if (interval.Defined()) {
                // convert interval from ms to timescale and expand from pts to dts
                const i64 searchDtsBegin = ConvertTimescaleFloor(interval->Begin.Value, Ti64TimeP::Timescale, timescale) - maxCto;
                const i64 searchDtsEnd = ConvertTimescaleCeil(interval->End.Value, Ti64TimeP::Timescale, timescale) - minCto;

                const auto getIndex = [samplesCount, allBLocksBegin, allBLocksEnd, &readDtsBlock](const i64 searchDts, TDtsIterator& it) -> ui64 {
                    if (it == allBLocksBegin) {
                        return 0;
                    } else if (it < allBLocksEnd && *it == searchDts) {
                        return readDtsBlock(it).IndexBegin;
                    } else {
                        it += -1;
                        const TDtsBlock block = readDtsBlock(it);
                        ui64 index;
                        if (block.Duration == 0) {
                            Y_ENSURE(block.IndexBegin + 1 == samplesCount);
                            index = (block.DtsBegin >= searchDts) ? block.IndexBegin : (block.IndexBegin + 1);
                        } else {
                            index = DivCeil(searchDts - block.DtsBegin + block.IndexBegin * (ui64)block.Duration, block.Duration);
                        }
                        return Min<ui64>(index, samplesCount);
                    }
                };

                blocksBegin = LowerBound(allBLocksBegin, allBLocksEnd, searchDtsBegin);
                indexBegin = getIndex(searchDtsBegin, blocksBegin);

                TDtsIterator blocksEnd = LowerBound(allBLocksBegin, allBLocksEnd, searchDtsEnd);
                indexEnd = getIndex(searchDtsEnd, blocksEnd);
            }

            Y_ENSURE(indexEnd >= indexBegin);

            if (indexBegin == indexEnd) {
                reader.SetPosition(theEnd);
                return;
            }

            // index range is known, now read data
            samples.resize(indexEnd - indexBegin);

            // fill dts
            {
                size_t index = indexBegin;
                TDtsIterator blockIt = blocksBegin;

                TDtsBlock nextBlock = readDtsBlock(blockIt);

                for (; index < indexEnd;) {
                    const TDtsBlock block = nextBlock;
                    ++blockIt;
                    if (blockIt == allBLocksEnd) {
                        nextBlock.IndexBegin = samplesCount;
                    } else {
                        nextBlock = readDtsBlock(blockIt);
                    }
                    const size_t blockIndexEnd = Min(nextBlock.IndexBegin, indexEnd);

                    i64 dts = block.DtsBegin;
                    if (index > block.IndexBegin) {
                        dts += block.Duration * (index - block.IndexBegin);
                    }

                    for (; index < blockIndexEnd; ++index, dts += block.Duration) {
                        auto& s = samples[index - indexBegin];
                        s.Dts = Ti64TimeP(dts);
                        s.CoarseDuration = Ti32TimeP(block.Duration);
                    }
                }
            }

            // fill cto
            if (ctoDefined) {
                reader.SetPosition(ctoBegin + ctoSize * indexBegin);
                for (auto& s : samples) {
                    i32 cto = 0;
                    EasyWorkIO(reader, cto);
                    s.Cto = Ti32TimeP(cto);
                }
            } else {
                Y_ENSURE(minCto == maxCto);
                for (auto& s : samples) {
                    s.Cto = Ti32TimeP(minCto);
                }
            }

            // convert cto and dts to packager timescale
            if (timescale != Ti64TimeP::Timescale) {
                for (auto& s : samples) {
                    const i64 dts = s.Dts.Value;
                    const i64 pts = s.Dts.Value + s.Cto.Value;
                    const i64 pDts = convertToP(dts);
                    const i64 pPts = convertToP(pts);
                    s.Dts = Ti64TimeP(pDts);
                    s.Cto = Ti32TimeP(pPts - pDts);
                    s.CoarseDuration = Ti32TimeP(Ti64TimeP(convertToP(dts + s.CoarseDuration.Value)) - s.Dts);
                }
            }

            // fill flags (by keyframes)
            if (keyframesCount == 0) {
                // all are keyframes
                for (auto& s : samples) {
                    s.Flags = 2 << 24;
                }
            } else {
                const TKeyframeIterator allKeyframesBegin = TKeyframeIterator(&reader, keyframesBegin);
                const TKeyframeIterator allKeyframesEnd = TKeyframeIterator(&reader, keyframesEnd);
                TKeyframeIterator kfit = LowerBound(allKeyframesBegin, allKeyframesEnd, indexBegin);
                for (ui32 index = indexBegin; index < indexEnd;) {
                    const ui32 kfIndex = Min(indexEnd, kfit < allKeyframesEnd ? *kfit : indexEnd);
                    ++kfit;

                    for (; index < kfIndex; ++index) {
                        samples[index - indexBegin].Flags = 1 << 16; // simple frame
                    }

                    if (index < indexEnd) {
                        samples[index - indexBegin].Flags = 2 << 24; // keyframe
                        ++index;
                    }
                }
            }

            // fill DataSize
            {
                reader.SetPosition(dataSizeBegin + datasizeSize * indexBegin);
                for (auto& s : samples) {
                    EasyWorkIO(reader, s.DataSize);
                }
            }

            // fill DataFileOffset
            {
                const TIdxIterator allDataBlocksBegin = TIdxIterator(&reader, dataBlocksBegin);
                const TIdxIterator allDataBlocksEnd = TIdxIterator(&reader, dataBlocksEnd);
                TIdxIterator blockIt = LowerBound(allDataBlocksBegin, allDataBlocksEnd, indexBegin);
                if (blockIt == allDataBlocksEnd || *blockIt > indexBegin) {
                    Y_ENSURE(allDataBlocksBegin < blockIt);
                    blockIt += -1;
                }

                TDataBlock nextBlock = readDataBlock(blockIt);
                if (nextBlock.IndexBegin != indexBegin) {
                    Y_ENSURE(nextBlock.IndexBegin < indexBegin);
                    reader.SetPosition(dataSizeBegin + nextBlock.IndexBegin * datasizeSize);
                    while (nextBlock.IndexBegin < indexBegin) {
                        ui32 size = 0;
                        EasyWorkIO(reader, size);
                        nextBlock.IndexBegin += 1;
                        nextBlock.DataBeginOffset += size;
                    }
                }

                for (ui32 index = indexBegin; index < indexEnd;) {
                    TDataBlock block = nextBlock;
                    ++blockIt;
                    if (blockIt == allDataBlocksEnd) {
                        nextBlock.IndexBegin = samplesCount;
                    } else {
                        nextBlock = readDataBlock(blockIt);
                    }
                    const size_t blockIndexEnd = Min(nextBlock.IndexBegin, indexEnd);

                    ui64 offset = block.DataBeginOffset;
                    for (; index < blockIndexEnd; ++index) {
                        auto& s = samples[index - indexBegin];
                        s.DataFileOffset = offset;
                        offset += s.DataSize;
                    }
                }
            }

            // actually DataBlob and DataBlobOffset can be left unitialised
            for (auto& s : samples) {
                s.DataBlob = nullptr;
                s.DataBlobOffset = 0;
            }

            reader.SetPosition(theEnd);
        }

        template <typename TIOWorker, typename T, typename... Args>
        inline void VectorWorkIO(TIOWorker& ioworker, TVector<T>& data, Args&... args) {
            size_t size = data.size();
            EasyWorkIO(ioworker, size);
            data.resize(size);

            for (T& v : data) {
                WorkIO(ioworker, v, args...);
            }
        }

        template <typename TIOWorker, typename... TArgs>
        inline void WorkIO(TIOWorker& ioworker, TMoovData& data, TArgs&... args) {
            VectorWorkIO(ioworker, data.TracksDataParams);
            VectorWorkIO(ioworker, data.TracksInfo);
            VectorWorkIO(ioworker, data.TracksSamples, args...);
            VectorWorkIO(ioworker, data.TracksTrackExtendsBoxes);
            VectorWorkIO(ioworker, data.TracksMoofTimescale);
        }

        // while reading moov in TMovieBox with this reader sample tables will not be read
        class TWierdBlobReader: public NMP4Muxer::TBlobReader {
        public:
            TWierdBlobReader(const TSimpleBlob& blob)
                : NMP4Muxer::TBlobReader(blob.begin(), blob.size())
            {
            }
        };

        inline bool Mp4ReadSkipSampleTableBox(TWierdBlobReader&) {
            return true;
        }
    }

    TFileMediaData LoadTFileMediaDataFromMoovData(const TMaybe<TIntervalP>& interval, void const* buffer, size_t bufferSize) {
        NMP4Muxer::TBlobReader reader((ui8 const*)buffer, bufferSize);

        TFileMediaData result;
        VectorWorkIO(reader, result.TracksDataParams);
        VectorWorkIO(reader, result.TracksInfo);
        VectorWorkIO(reader, result.TracksSamples, interval);
        VectorWorkIO(reader, result.TracksTrackExtendsBoxes);
        VectorWorkIO(reader, result.TracksMoofTimescale);

        result.Duration = Ti64TimeP(0);
        for (const auto& ts : result.TracksSamples) {
            if (!ts.empty()) {
                result.Duration = Max<Ti64TimeP>(result.Duration, ts.back().Dts + ts.back().CoarseDuration);
            }
        }

        return result;
    }

    // faster, becasue it does not copy moov sample tables data several times while converting, but instead convert while reading without extra copies
    TBuffer SaveTFileMediaDataFast(const TSimpleBlob& data, const bool kalturaMode) {
        TWierdBlobReader reader(data);

        NMP4Muxer::TMovieBox moov;

        // here sample tables will not be read (because of TWierdBlobReader)
        moov.WorkIO(reader);

        // so md will have all, but samples
        TMoovData md = ReadMoov(moov, kalturaMode);

        NMP4Muxer::TBufferWriter fakeWriter;

        WorkIO(fakeWriter, md);

        ui64 reserveSize = fakeWriter.GetPosition();
        for (const auto& ts : md.TracksSamples) {
            NMP4Muxer::TTrackBox const* trackBoxPtr = nullptr;
            for (const auto& tb : moov.TrackBoxes) {
                if (tb.TrackHeaderBox->TrackID == ts.TrackID) {
                    Y_ENSURE(!trackBoxPtr);
                    trackBoxPtr = &tb;
                }
            }
            Y_ENSURE(trackBoxPtr);
            const NMP4Muxer::TSampleTableBox& stbl = trackBoxPtr->MediaBox->MediaInformationBox.SampleTableBox;

            const ui32 samplesCount = stbl.SampleSizeBox.Defined() ? stbl.SampleSizeBox->SamplesCount : stbl.CompactSampleSizeBox->SamplesCount;

            // samples data size
            reserveSize += 4 * samplesCount;

            // dts blocks
            reserveSize += stbl.TimeToSampleBox->EntriesCount * 16;

            // cto
            reserveSize += stbl.CompositionTimeToSampleBox.Defined() ? 4 * samplesCount : 0;

            // keyframes
            reserveSize += stbl.SyncSampleBox.Defined() ? stbl.SyncSampleBox->NumbersCount * 4 : 0;

            // datablocks
            const ui32 chunksCount = stbl.ChunkOffsetBox.Defined() ? stbl.ChunkOffsetBox->ChunksCount : stbl.ChunkLargeOffsetBox->ChunksCount;
            reserveSize += 12 * (chunksCount + 1 + samplesCount / TMoovData::TTrackSamples::DataBlockMaxSamplesCount);
        }

        NMP4Muxer::TBufferWriter writer;
        writer.Buffer().Reserve(reserveSize);

        WorkIO(writer, md, reader, moov);

        Y_ENSURE(writer.Buffer().size() <= reserveSize);

        return std::move(writer.Buffer());
    }

    // slower, but simpler
    TBuffer SaveTFileMediaDataSlow(const TSimpleBlob& data, const bool kalturaMode) {
        NMP4Muxer::TBlobReader reader(data.begin(), data.size());
        NMP4Muxer::TMovieBox moov;
        moov.WorkIO(reader);
        TMoovData md = ReadMoov(moov, kalturaMode);

        NMP4Muxer::TFakeWriter fakeWriter;

        WorkIO(fakeWriter, md);

        NMP4Muxer::TBufferWriter writer;

        const size_t reserveSize = fakeWriter.GetPosition();
        writer.Buffer().Reserve(reserveSize);

        WorkIO(writer, md);

        Y_ENSURE(writer.Buffer().size() == reserveSize);

        return std::move(writer.Buffer());
    }

    // run both slow and fast versions and check they make equal results
    TBuffer SaveTFileMediaDataCheck(const TSimpleBlob& data, const bool kalturaMode) {
        TBuffer slow = SaveTFileMediaDataSlow(data, kalturaMode);
        TBuffer fast = SaveTFileMediaDataFast(data, kalturaMode);

        Y_ENSURE(slow.Size() == fast.Size());
        Y_ENSURE(std::memcmp(slow.Data(), fast.Data(), fast.Size()) == 0);

        return slow;
    }

}
