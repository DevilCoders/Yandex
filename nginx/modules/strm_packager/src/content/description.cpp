#include <nginx/modules/strm_packager/src/content/description.h>

#include <nginx/modules/strm_packager/src/base/http_error.h>

namespace NStrm::NPackager::NDescription {
    const NFb::TTrack* GetVideoTrack(const TDescription* description, ui64 id) {
        for (const auto& set : *description->VideoSets()) {
            for (const auto& video : *set->Tracks()) {
                if (video->Id() == id) {
                    return video;
                }
            }
        }
        Y_ENSURE_EX(false, THttpError(404, TLOG_WARNING) << "failed to find video track with id " << id);
    }

    const NFb::TTrack* GetAudioTrack(const TDescription* description, ui64 id) {
        for (const auto& set : *description->AudioSets()) {
            for (const auto& audio : *set->Tracks()) {
                if (audio->Id() == id) {
                    return audio;
                }
            }
        }
        Y_ENSURE_EX(false, THttpError(404, TLOG_WARNING) << "failed to find audio track with id " << id);
    }

    void AddInitSourceInfo(
        const flatbuffers::Vector<flatbuffers::Offset<NStrm::NPackager::NFb::TSegment>>* segments,
        TVector<TSourceInfo>& sourceInfos)
    {
        sourceInfos.emplace_back(TSourceInfo{
            .Path = TString(segments->begin()->Path()->c_str()),
        });
    }

    void AddSourceInfos(
        const flatbuffers::Vector<flatbuffers::Offset<NStrm::NPackager::NFb::TSegment>>* segments,
        const TIntervalMs interval,
        const Ti64TimeMs offset,
        TVector<TSourceInfo>& sourceInfos)
    {
        const auto getSegmentOffset = [](const NFb::TSegment* segment) -> i64 { return segment->Offset(); };
        auto segment = UpperBoundBy(segments->begin(), segments->end(), interval.Begin.Value, getSegmentOffset);

        if (segments->begin() < segment) {
            --segment;
        }

        for (; segment != segments->end(); ++segment) {
            auto resultInterval = TIntervalMs{
                .Begin = (Ti64TimeMs)Max<i64>(0, interval.Begin.Value - segment->Offset()),
                .End = (Ti64TimeMs)Min<i64>(segment->Duration(), interval.End.Value - segment->Offset()),
            };

            if (resultInterval.End <= resultInterval.Begin) {
                break;
            }

            sourceInfos.emplace_back(TSourceInfo{
                .Path = TString(segment->Path()->c_str()),
                .Interval = resultInterval,
                .Offset = Ti64TimeMs(segment->Offset()) + offset,
            });
        }
    }
}
