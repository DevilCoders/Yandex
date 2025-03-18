#include <nginx/modules/strm_packager/src/content/vod_description_details.h>

#include <strm/plgo/pkg/proto/vod/description/v1/description.pb.h>
#include <strm/plgo/pkg/proto/vod/description/v2/description.pb.h>

#include <nginx/modules/strm_packager/src/fbs/description.fbs.h>

#include <contrib/libs/protobuf/src/google/protobuf/util/json_util.h>
#include <contrib/libs/protobuf/src/google/protobuf/util/time_util.h>
#include <library/cpp/json/json_reader.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NStrm::NPackager::NVodDescriptionDetails {
    TString SegmentPathFromTemplate(const TProtoStringType& templatePath, ui64 segmentStartTS, ui64 segmentEndTS, ui64 segmentIndex) {
        TProtoStringType segmentPath;
        segmentPath = google::protobuf::StringReplace(templatePath, "${end_time}", google::protobuf::SimpleItoa(segmentEndTS), true);
        segmentPath = google::protobuf::StringReplace(segmentPath, "${start_time}", google::protobuf::SimpleItoa(segmentStartTS), true);
        segmentPath = google::protobuf::StringReplace(segmentPath, "${index}", google::protobuf::SimpleItoa(segmentIndex), true);
        return segmentPath;
    }

    ui64 ParseDuration(float duration) {
        return ui64(duration * 1000);
    }

    TString ParsePath(TString path) {
        if (path.StartsWith('/')) {
            return path;
        }
        return TStringBuilder() << "/" << path;
    }

    google::protobuf::util::Status JsonToProto(const TString& jsonString, google::protobuf::Message& message, bool ignoreUnknownFields) {
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = ignoreUnknownFields;
        return google::protobuf::util::JsonStringToMessage(jsonString, &message, options);
    }

    void FillSegmentsWithStubs(
        TVector<flatbuffers::Offset<NFb::TSegment>>& segments,
        ui64 stubEndIntervalTS,
        const description::v2::StubInfo& stubInfo,
        const ui64 contentStartTS,
        const ui64 segmentDuration,
        ui64& durationBefore,
        const ui64 globalOffset,
        flatbuffers::FlatBufferBuilder& builder) {
        const ui64 stubDuration = stubInfo.Getduration();
        const TString stubPath = ParsePath(stubInfo.Getpath());

        for (;;) {
            const ui64 currentTS = contentStartTS + durationBefore;
            Y_ENSURE(currentTS <= stubEndIntervalTS);
            if (currentTS == stubEndIntervalTS) {
                break;
            }

            auto duration = Min(stubEndIntervalTS - currentTS, stubDuration);

            if (currentTS % segmentDuration != 0) {
                duration = Min(duration, segmentDuration - (currentTS % segmentDuration));
            }

            const auto& path = stubPath;
            const auto offset = currentTS - contentStartTS + globalOffset;

            segments.emplace_back(NFb::CreateTSegment(builder, builder.CreateString(path), duration, offset));

            durationBefore += duration;
        }
    }

    void FillSegmentsFromIntervalTemplate(
        TVector<flatbuffers::Offset<NFb::TSegment>>& segments,
        const ::description::v2::SegmentTemplate& intervalTemplate,
        const ui64 contentStartTS,
        const ui64 templateStartIndex,
        ui64& durationBefore,
        const ui64 globalOffset,
        flatbuffers::FlatBufferBuilder& builder) {
        const auto templateSegmentDuration = intervalTemplate.segment_duration();
        const auto& templatePath = ParsePath(intervalTemplate.Getpath_template());

        for (ui64 i = 0; i < intervalTemplate.Getcount(); ++i) {
            const auto segmentIndex = templateStartIndex + i;
            const auto segmentStartTS = contentStartTS + durationBefore;
            const auto segmentEndTS = segmentStartTS + templateSegmentDuration;

            const auto segmentDuration = templateSegmentDuration;
            const auto segmentPath = SegmentPathFromTemplate(templatePath, segmentStartTS, segmentEndTS, segmentIndex);
            const auto offset = segmentStartTS - contentStartTS + globalOffset;

            segments.emplace_back(NFb::CreateTSegment(builder, builder.CreateString(segmentPath), segmentDuration, offset));

            durationBefore += templateSegmentDuration;
        }
    }

    auto ParseV1Segments(const google::protobuf::RepeatedPtrField<description::v1::Segment>& segments,
                         ui64& durationBefore,
                         flatbuffers::FlatBufferBuilder& builder) {
        TVector<flatbuffers::Offset<NFb::TSegment>> resultSegments;
        for (const auto& segment : segments) {
            const auto duration = ParseDuration(segment.GetDuration());
            const auto path = ParsePath(segment.GetPath());
            const auto offset = durationBefore;
            resultSegments.emplace_back(NFb::CreateTSegment(builder, builder.CreateString(path), duration, offset));
            durationBefore += duration;
        }
        Y_ENSURE(!resultSegments.empty());
        Y_ENSURE(durationBefore > 0);
        return resultSegments;
    }

    auto ParseV2Segments(const google::protobuf::RepeatedPtrField<description::v2::Segment>& segments,
                         ui64& durationBefore,
                         flatbuffers::FlatBufferBuilder& builder) {
        TVector<flatbuffers::Offset<NFb::TSegment>> resultSegments;
        for (const auto& segment : segments) {
            const auto duration = ParseDuration(segment.Getduration());
            const auto path = ParsePath(segment.Getpath());
            const auto offset = durationBefore;
            resultSegments.emplace_back(NFb::CreateTSegment(builder, builder.CreateString(path), duration, offset));
            durationBefore += duration;
        }
        Y_ENSURE(!resultSegments.empty());
        Y_ENSURE(durationBefore > 0);
        return resultSegments;
    }

    auto ParseV2Intervals(const google::protobuf::RepeatedPtrField<description::v2::SegmentInterval>& intervals,
                          const description::v2::StubInfo& stubInfo,
                          const google::protobuf::uint64 segmentDuration,
                          ui64& durationBefore,
                          flatbuffers::FlatBufferBuilder& builder) {
        TVector<flatbuffers::Offset<NFb::TSegment>> resultSegments;
        ui64 contentStartTS = intervals[0].Getstart_time();

        const auto globalOffset = contentStartTS % segmentDuration;

        for (const auto& interval : intervals) {
            const ui64 stubIntervalEndTS = interval.Getstart_time();

            FillSegmentsWithStubs(
                resultSegments,
                stubIntervalEndTS,
                stubInfo,
                contentStartTS,
                segmentDuration,
                durationBefore,
                globalOffset,
                builder);
            Y_ENSURE(interval.Getstart_time() == contentStartTS + durationBefore);

            auto templateStartIndex = interval.Getstart_index();
            for (const auto& intervalTemplate : interval.Gettemplates()) {
                FillSegmentsFromIntervalTemplate(
                    resultSegments,
                    intervalTemplate,
                    contentStartTS,
                    templateStartIndex,
                    durationBefore,
                    globalOffset,
                    builder);
                templateStartIndex += intervalTemplate.Getcount();
            }
            Y_ENSURE(interval.Getend_time() == contentStartTS + durationBefore);
        }

        // after range for intervals duration before will be contentEndTime - contentStartTime
        // should add duration part of first non-multiple segment for right duration
        durationBefore += globalOffset;

        Y_ENSURE(!resultSegments.empty());
        Y_ENSURE(durationBefore > 0);
        return resultSegments;
    }

    void ParseV1VodDescription(const TString& jsonString, flatbuffers::FlatBufferBuilder& builder) {
        description::v1::Description desc;
        const auto status = JsonToProto(jsonString, desc, true);
        Y_ENSURE(status.ok(), "failed to parse proto-json description");

        TVector<flatbuffers::Offset<NFb::TTrackSet>> videoSets;
        for (const auto& video : desc.GetVideos()) {
            ui64 duration = 0;
            const auto segments = ParseV1Segments(video.GetSegments(), duration, builder);

            TStringBuilder params;
            params
                << "|" << video.GetWidth()
                << "|" << video.GetHeight()
                << "|" << video.GetFramerate()
                << "|" // empty codec
                << "|" << video.GetCodecs()
                << "|0-" // empty codec_info
                << "|";

            TVector<flatbuffers::Offset<NFb::TTrack>> videos{
                NFb::CreateTTrack(
                    builder,
                    video.GetID(),
                    builder.CreateVector(segments),
                    duration,
                    builder.CreateString(params))};

            videoSets.emplace_back(NFb::CreateTTrackSet(
                builder,
                builder.CreateString(""), // = lang
                builder.CreateString(video.GetLabel()),
                builder.CreateVector(videos)));
        }

        TVector<flatbuffers::Offset<NFb::TTrackSet>> audioSets;
        for (const auto& audio : desc.GetAudios()) {
            ui64 duration = 0;
            const auto segments = ParseV1Segments(audio.GetSegments(), duration, builder);

            TStringBuilder params;
            params
                << "|" << audio.GetSamplingRate()
                << "|" << audio.GetCodecs()
                << "|"   // empty codec
                << "|0"  // zero channels
                << "|0-" // empty dec3 info
                << "|";

            TVector<flatbuffers::Offset<NFb::TTrack>> audios{
                NFb::CreateTTrack(
                    builder,
                    audio.GetID(),
                    builder.CreateVector(segments),
                    duration,
                    builder.CreateString(params))};

            audioSets.emplace_back(NFb::CreateTTrackSet(
                builder,
                builder.CreateString(audio.GetLanguage()),
                builder.CreateString(audio.GetLabel()),
                builder.CreateVector(audios)));
        }

        Y_ENSURE(!audioSets.empty() || !videoSets.empty());

        const auto description = NFb::CreateTDescription(
            builder,
            builder.CreateVector(videoSets),
            builder.CreateVector(audioSets),
            desc.GetSegmentDuration(),
            /* WithDRM = */ false);

        builder.Finish(description);
    }

    void ParseV2VodDescription(const TString& jsonString, flatbuffers::FlatBufferBuilder& builder) {
        description::v2::Description desc;
        const auto status = JsonToProto(jsonString, desc, true);
        Y_ENSURE(status.ok(), "failed to parse proto-json description");

        TVector<flatbuffers::Offset<NFb::TTrackSet>> videoSets;
        for (const auto& set : desc.Getvideo_rendition_sets()) {
            TVector<flatbuffers::Offset<NFb::TTrack>> videos;
            for (const auto& video : set.Getvideos()) {
                ui64 duration = 0;
                TVector<flatbuffers::Offset<NFb::TSegment>> segments;

                const auto& videoIntervals = video.Getsegment_intervals();
                if (!videoIntervals.empty()) {
                    segments = ParseV2Intervals(videoIntervals, video.Getstub(), desc.Getsegment_duration(), duration, builder);
                } else {
                    segments = ParseV2Segments(video.Getsegments(), duration, builder);
                }

                TString codec;
                TString codec_info;
                switch (video.GetCodecInfoCase()) {
                    case description::v2::Video::kCodecInfoH264:
                        codec = ToString(video.Getcodec_info_h264().Getcodec());
                        codec_info = video.Getcodec_info_h264().Getsps() + "|" + video.Getcodec_info_h264().Getpps();
                        break;

                    case description::v2::Video::kCodecInfoH265:
                        codec = ToString(video.Getcodec_info_h265().Getcodec());
                        break;

                    case description::v2::Video::kCodecInfoVp9:
                        codec = ToString(video.Getcodec_info_vp9().Getcodec());
                        break;

                    case description::v2::Video::CODEC_INFO_NOT_SET:
                        codec = ToString(video.Getcodec());
                        codec_info = video.Getsps() + "|" + video.Getpps();
                        break;
                }

                TStringBuilder params;
                params
                    << "|" << set.Getwidth()
                    << "|" << set.Getheight()
                    << "|" << set.Getframerate()
                    << "|" << std::move(codec)
                    << "|" << video.Getcodecs()
                    << "|" << codec_info.length() << "-" << std::move(codec_info)
                    << "|";

                videos.emplace_back(NFb::CreateTTrack(
                    builder,
                    video.Getid(),
                    builder.CreateVector(segments),
                    duration,
                    builder.CreateString(params)));
            }

            Y_ENSURE(!videos.empty());
            videoSets.emplace_back(NFb::CreateTTrackSet(
                builder,
                builder.CreateString(""), // = lang
                builder.CreateString(set.Getlabel()),
                builder.CreateVector(videos)));
        }

        TVector<flatbuffers::Offset<NFb::TTrackSet>> audioSets;
        for (const auto& set : desc.Getaudio_rendition_sets()) {
            TVector<flatbuffers::Offset<NFb::TTrack>> audios;
            for (const auto& audio : set.Getaudios()) {
                ui64 duration = 0;
                TVector<flatbuffers::Offset<NFb::TSegment>> segments;

                const auto& audioIntervals = audio.Getsegment_intervals();
                if (!audioIntervals.empty()) {
                    segments = ParseV2Intervals(audioIntervals, audio.Getstub(), desc.Getsegment_duration(), duration, builder);
                } else {
                    segments = ParseV2Segments(audio.Getsegments(), duration, builder);
                }

                TString codec;
                TString dec3Info;
                switch (audio.GetCodecInfoCase()) {
                    case description::v2::Audio::kCodecInfoEc3:
                        codec = ToString(audio.Getcodec_info_ec3().Getcodec());
                        dec3Info = audio.Getcodec_info_ec3().Getdec3_payload();
                        break;

                    case description::v2::Audio::kCodecInfoAac:
                        codec = ToString(audio.Getcodec_info_aac().Getcodec());
                        break;

                    case description::v2::Audio::kCodecInfoMp3:
                        codec = ToString(audio.Getcodec_info_mp3().Getcodec());
                        break;

                    case description::v2::Audio::kCodecInfoOpus:
                        codec = ToString(audio.Getcodec_info_opus().Getcodec());
                        break;

                    case description::v2::Audio::kCodecInfoAc3:
                        codec = ToString(audio.Getcodec_info_ac3().Getcodec());
                        break;

                    case description::v2::Audio::CODEC_INFO_NOT_SET:
                        codec = ToString(audio.Getcodec());
                        break;
                }

                TStringBuilder params;
                params
                    << "|" << audio.Getsampling_rate()
                    << "|" << audio.Getcodecs()
                    << "|" << std::move(codec)
                    << "|" << set.Getchannels()
                    << "|" << dec3Info.length() << "-" << std::move(dec3Info)
                    << "|";

                audios.emplace_back(NFb::CreateTTrack(
                    builder,
                    audio.Getid(),
                    builder.CreateVector(segments),
                    duration,
                    builder.CreateString(params)));
            }

            Y_ENSURE(!audios.empty());
            audioSets.emplace_back(NFb::CreateTTrackSet(
                builder,
                builder.CreateString(set.Getlanguage()),
                builder.CreateString(set.Getlabel()),
                builder.CreateVector(audios)));
        }

        Y_ENSURE(!audioSets.empty() || !videoSets.empty());

        const auto description = NFb::CreateTDescription(
            builder,
            builder.CreateVector(videoSets),
            builder.CreateVector(audioSets),
            desc.Getsegment_duration(),
            /* WithDRM = */ desc.has_drm());

        builder.Finish(description);
    }
}
