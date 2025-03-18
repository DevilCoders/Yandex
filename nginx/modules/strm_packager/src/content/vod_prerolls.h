#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/content/description.h>

#include <util/generic/string.h>
#include <util/generic/maybe.h>

namespace NStrm::NPackager {
    TVector<TString> GetPrerolls(const TRequestWorker& request);

    void CheckPrerollsConfig(const TLocationConfig& config);

    ui32 SelectPrerollTrackId(
        decltype(&NDescription::TDescription::VideoSets) descGetTracksSet,
        const NDescription::TDescription& prerollDescription,
        const NDescription::TDescription& mainDescription,
        const ui32 mainIndex);
}
