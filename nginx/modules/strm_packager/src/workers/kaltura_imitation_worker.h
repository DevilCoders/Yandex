#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>
#include <nginx/modules/strm_packager/src/common/track_data.h>
#include <nginx/modules/strm_packager/src/content/live_uri.h>

namespace NStrm::NPackager {
    // could be updgraded to do everything kaltura do, but most likely will be used only for live subtitles, that kaltura cant do
    class TKalturaImitationWorker: public TRequestWorker {
    public:
        TKalturaImitationWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

    private:
        using TChunkInfo = TCommonChunkInfo;

        TChunkInfo MakeChunkInfo() const;

        void Work() override;
    };
}
