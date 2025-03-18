#pragma once

#include <nginx/modules/strm_packager/src/common/track_data.h>
#include <nginx/modules/strm_packager/src/fbs/description.fbs.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NStrm::NPackager::NDescription {
    using TDescription = NFb::TDescription;

    struct TSourceInfo {
        TString Path;
        TIntervalMs Interval;
        Ti64TimeMs Offset;
    };

    const NFb::TTrack* GetVideoTrack(const TDescription* description, ui64 id);
    const NFb::TTrack* GetAudioTrack(const TDescription* description, ui64 id);

    void AddInitSourceInfo(
        const flatbuffers::Vector<flatbuffers::Offset<NStrm::NPackager::NFb::TSegment>>* segments,
        TVector<NDescription::TSourceInfo>& sourceInfos);

    void AddSourceInfos(
        const flatbuffers::Vector<flatbuffers::Offset<NStrm::NPackager::NFb::TSegment>>* segments,
        const TIntervalMs interval,
        const Ti64TimeMs offset,
        TVector<TSourceInfo>& sourceInfos);
}
