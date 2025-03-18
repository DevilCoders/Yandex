#include <nginx/modules/strm_packager/src/common/mp4_common.h>

#include <util/generic/overloaded.h>

namespace NStrm::NPackager {
    TSampleData::TDataParams Mp4TrackParams2TrackDataParams(const TTrackInfo::TParams& params) {
        return std::visit(
            TOverloaded{
                [](const TTrackInfo::TTimedMetaId3Params&) {
                    return TSampleData::TDataParams{
                        .Format = TSampleData::TDataParams::EFormat::TimedMetaId3,
                    };
                },
                [](const TTrackInfo::TAudioParams& ap) {
                    TSampleData::TDataParams result;

                    switch (ap.BoxType) {
                        case 'mp4a':
                            switch (ap.CodecType) {
                                case TTrackInfo::TAudioParams::ECodecType::AacMain:
                                case TTrackInfo::TAudioParams::ECodecType::AacLC:
                                case TTrackInfo::TAudioParams::ECodecType::AacSSR:
                                case TTrackInfo::TAudioParams::ECodecType::AacLTP:
                                    result.Format = TSampleData::TDataParams::EFormat::AudioAccRDB;
                                    break;

                                case TTrackInfo::TAudioParams::ECodecType::Mp3Mpeg1a:
                                case TTrackInfo::TAudioParams::ECodecType::Mp3Mpeg2a:
                                    result.Format = TSampleData::TDataParams::EFormat::AudioMp3;
                                    break;

                                default:
                                    Y_ENSURE(false, "Mp4TrackParams2TrackDataParams: unsupported codec type " << ap.CodecType);
                            }
                            break;

                        case 'ac-3':
                            Y_ENSURE(ap.CodecType == TTrackInfo::TAudioParams::ECodecType::AC3);
                            Y_ENSURE(ap.AC3SpecificBox.Defined() && ap.AC3SpecificBox->Type == 'dac3');
                            result.Format = TSampleData::TDataParams::EFormat::AudioAc3;
                            break;

                        case 'ec-3':
                            Y_ENSURE(ap.CodecType == TTrackInfo::TAudioParams::ECodecType::EAC3);
                            Y_ENSURE(ap.EC3SpecificBox.Defined() && ap.EC3SpecificBox->Type == 'dec3');
                            result.Format = TSampleData::TDataParams::EFormat::AudioEAc3;
                            break;

                        case 'Opus':
                            Y_ENSURE(ap.CodecType == TTrackInfo::TAudioParams::ECodecType::Opus);
                            Y_ENSURE(ap.OpusSpecificBox.Defined() && ap.OpusSpecificBox->Type == 'dOps');
                            result.Format = TSampleData::TDataParams::EFormat::AudioOpus;
                            break;

                        default:
                            Y_ENSURE(false, "Mp4TrackParams2TrackDataParams: unsupported box type " << ap.BoxType);
                    }

                    return result;
                },
                [](const TTrackInfo::TVideoParams& vp) {
                    if (vp.CodecParamsBox->Type == 'avcC') {
                        return TSampleData::TDataParams{
                            .Format = TSampleData::TDataParams::EFormat::VideoAvcc,
                            .NalUnitLengthSize = NAvcCBox::GetNaluSizeLength(vp.CodecParamsBox->Data),
                        };
                    } else if (vp.CodecParamsBox->Type == 'hvcC') {
                        return TSampleData::TDataParams{
                            .Format = TSampleData::TDataParams::EFormat::VideoHvcc,
                            .NalUnitLengthSize = NHvcCBox::GetNaluSizeLength(vp.CodecParamsBox->Data),
                        };
                    } else if (vp.CodecParamsBox->Type == 'vpcC') {
                        // no need to parse
                        return TSampleData::TDataParams{
                            .Format = TSampleData::TDataParams::EFormat::VideoVpcc,
                        };
                    } else {
                        Y_ENSURE(false, "Mp4TrackParams2TrackDataParams: unknown video codec params box");
                    }
                },
                [](const TTrackInfo::TSubtitleParams& sp) {
                    switch (sp.Type) {
                        case TTrackInfo::TSubtitleParams::EType::RawText:
                            return TSampleData::TDataParams{
                                .Format = TSampleData::TDataParams::EFormat::SubtitleRawText,
                            };

                        case TTrackInfo::TSubtitleParams::EType::TTML:
                            return TSampleData::TDataParams{
                                .Format = TSampleData::TDataParams::EFormat::SubtitleTTML,
                            };
                    }
                }},
            params);
    }

    TMoovData ReadMoov(const NMP4Muxer::TMovieBox& moov, const bool kalturaMode) {
        using TTrackBox = NMP4Muxer::TTrackBox;
        using TTrackMoovData = NMP4Muxer::TTrackMoovData;
        using TEditListBox = NMP4Muxer::TEditListBox;
        using TSampleTableBox = NMP4Muxer::TSampleTableBox;
        using TTimeToSampleBox = NMP4Muxer::TTimeToSampleBox;
        using TCompositionTimeToSampleBox = NMP4Muxer::TCompositionTimeToSampleBox;
        using TSampleToChunkBox = NMP4Muxer::TSampleToChunkBox;

        using TTrackExtendsBox = NMP4Muxer::TTrackExtendsBox;

        TMoovData result;

        TTrackBox const* tracksFromMoov[moov.TrackBoxes.size()];

        size_t tracksCount = 0;

        for (const TTrackBox& tr : moov.TrackBoxes) {
            if (!tr.MediaBox->MediaInformationBox.VideoMediaHeaderBox.Defined() &&
                !tr.MediaBox->MediaInformationBox.SoundMediaHeaderBox.Defined())
            {
                continue; // meta track
            }

            tracksFromMoov[tracksCount] = &tr;
            ++tracksCount;
        }

        Sort(
            tracksFromMoov,
            tracksFromMoov + tracksCount,
            [](TTrackBox const* a, TTrackBox const* b) {
                return a->TrackHeaderBox->TrackID < b->TrackHeaderBox->TrackID;
            });

        result.TracksDataParams.resize(tracksCount);
        result.TracksInfo.resize(tracksCount);
        result.TracksSamples.resize(tracksCount);

        const ui32 movieHeaderTimescale = moov.MovieHeaderBox->Timescale;

        for (size_t i = 0; i < tracksCount; ++i) {
            const TTrackBox& trackbox = *tracksFromMoov[i];
            TTrackMoovData thisMoovData(trackbox);

            result.TracksInfo[i].Language = thisMoovData.Language;
            result.TracksInfo[i].Name = thisMoovData.Name;
            result.TracksInfo[i].Params = thisMoovData.Params;
            result.TracksDataParams[i] = Mp4TrackParams2TrackDataParams(result.TracksInfo[i].Params);

            // fill samples
            TMoovData::TTrackSamples& samples = result.TracksSamples[i];
            samples.TrackID = trackbox.TrackHeaderBox->TrackID;
            samples.DtsOffsetByEditListBox = 0;

            samples.MinCto = 0;
            samples.MaxCto = 0;
            samples.Timescale = thisMoovData.Timescale;

            i64 duration = 0;
            // read edit list box
            if (trackbox.EditBox.Defined() && trackbox.EditBox->EditListBox.Defined()) {
                // something more complicated than single offset is not supported
                TMaybe<i64> offset;

                i64 time = 0;
                for (const TEditListBox::TEntry& e : trackbox.EditBox->EditListBox->Entries) {
                    if (e.MediaRateInteger == 1 && e.MediaTime != -1) {
                        Y_ENSURE(e.MediaTime >= 0);
                        if (offset.Empty()) {
                            offset = time - e.MediaTime;
                        } else {
                            Y_ENSURE(time = e.MediaTime + *offset, "elst is more complicated than single offset");
                        }
                    }

                    // e.MediaTime use timescale for this track
                    // but e.SegmentDuration use timescale from moov ( movieHeaderTimescale )
                    // so we need to convert:
                    time += (e.SegmentDuration * samples.Timescale + movieHeaderTimescale / 2) / movieHeaderTimescale;
                }
                duration = offset.GetOrElse(0);
            }

            if (kalturaMode) {
                duration = Max<i64>(0, duration);
            }

            samples.DtsOffsetByEditListBox = duration;

            const TSampleTableBox& stbl = trackbox.MediaBox->MediaInformationBox.SampleTableBox;

            // samples count
            samples.SamplesCount = stbl.SampleSizeBox.Defined() ? stbl.SampleSizeBox->SamplesSize.size() : stbl.CompactSampleSizeBox->SamplesSize.size();
            if (samples.SamplesCount == 0) {
                continue;
            }

            // samples data size
            if (stbl.SampleSizeBox.Defined()) {
                samples.DataSize.assign(stbl.SampleSizeBox->SamplesSize.begin(), stbl.SampleSizeBox->SamplesSize.end());
            } else {
                samples.DataSize.assign(stbl.CompactSampleSizeBox->SamplesSize.begin(), stbl.CompactSampleSizeBox->SamplesSize.end());
            }

            // dts and durations
            {
                ui32 sampleIndex = 0;
                const TTimeToSampleBox& stts = *stbl.TimeToSampleBox;
                samples.DtsBlocks.resize(stts.Entries.size());

                for (size_t j = 0; j < stts.Entries.size(); ++j) {
                    const auto& src = stts.Entries[j];
                    auto& dst = samples.DtsBlocks[j];

                    dst.IndexBegin = sampleIndex;
                    dst.Duration = src.SampleDelta;
                    dst.DtsBegin = duration;
                    sampleIndex += src.SampleCount;
                    duration += src.SampleCount * src.SampleDelta;
                }
                Y_ENSURE(sampleIndex == samples.SamplesCount);
            }

            // fill CTO
            if (stbl.CompositionTimeToSampleBox.Defined()) {
                const TCompositionTimeToSampleBox& ctts = *stbl.CompositionTimeToSampleBox;

                samples.Cto.ConstructInPlace(samples.SamplesCount);
                auto& scto = *samples.Cto;

                size_t index = 0;
                for (const TCompositionTimeToSampleBox::TEntry& ent : ctts.Entries) {
                    for (size_t k = 0; k < ent.SampleCount; ++k) {
                        scto[index] = ent.SampleOffset;
                        ++index;
                    }
                    samples.MinCto = Min(samples.MinCto, ent.SampleOffset);
                    samples.MaxCto = Max(samples.MaxCto, ent.SampleOffset);
                }
                Y_ENSURE(index == samples.SamplesCount);

                if (samples.MinCto == samples.MaxCto) {
                    samples.Cto.Clear();
                }
            }

            // fill data blocks
            {
                const size_t chunksCount = stbl.ChunkOffsetBox.Defined() ? stbl.ChunkOffsetBox->ChunkOffsets.size() : stbl.ChunkLargeOffsetBox->ChunkOffsets.size();

                const TSampleToChunkBox& stsc = *stbl.SampleToChunkBox;
                auto stscIt = stsc.Entries.begin();
                Y_ENSURE(stscIt->FirstChunk == 1);

                size_t sampleIndex = 0;
                for (size_t chunkIndex = 0; chunkIndex < chunksCount; ++chunkIndex) {
                    ui64 dataOffset = stbl.ChunkOffsetBox.Defined() ? stbl.ChunkOffsetBox->ChunkOffsets[chunkIndex] : stbl.ChunkLargeOffsetBox->ChunkOffsets[chunkIndex];

                    if (stscIt + 1 < stsc.Entries.end() && chunkIndex + 1 == (stscIt + 1)->FirstChunk) {
                        ++stscIt;
                    }

                    ui32 samplesInChunk = stscIt->SamplesPerChunk;

                    while (samplesInChunk > 0) {
                        const ui32 samplesInBlock = Min(samplesInChunk, TMoovData::TTrackSamples::DataBlockMaxSamplesCount);
                        samples.DataBlocks.push_back(TMoovData::TTrackSamples::TDataBlock{
                            .IndexBegin = (ui32)sampleIndex,
                            .DataBeginOffset = dataOffset,
                        });

                        for (ui32 i = 0; i < samplesInBlock; ++i, ++sampleIndex) {
                            dataOffset += samples.DataSize[sampleIndex];
                        }

                        samplesInChunk -= samplesInBlock;
                    }
                }

                Y_ENSURE(sampleIndex == samples.SamplesCount);
            }

            // keyframes
            if (stbl.SyncSampleBox.Defined()) {
                samples.Keyframes.reserve(stbl.SyncSampleBox->SampleNumber.size());
                for (const ui32& v : stbl.SyncSampleBox->SampleNumber) {
                    Y_ENSURE(v >= 1 && v <= samples.SamplesCount);
                    samples.Keyframes.push_back(v - 1);
                }
            }
        }

        // if mvex exist
        if (moov.MovieExtendsBox.Defined()) {
            result.TracksTrackExtendsBoxes.resize(tracksCount);
            result.TracksMoofTimescale.resize(tracksCount, 0);

            for (const TTrackExtendsBox& trex : moov.MovieExtendsBox->TrackExtendsBoxes) {
                for (size_t trackIndex = 0; trackIndex < tracksCount; ++trackIndex) {
                    const TTrackBox& trackbox = *tracksFromMoov[trackIndex];

                    if (trackbox.TrackHeaderBox->TrackID == trex.TrackID) {
                        Y_ENSURE(result.TracksMoofTimescale[trackIndex] == 0);
                        result.TracksMoofTimescale[trackIndex] = trackbox.MediaBox->MediaHeaderBox.Timescale;

                        result.TracksTrackExtendsBoxes[trackIndex] = trex;
                    }
                }
            }
        }

        return result;
    }
}
