#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/drm_info_util.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>
#include <nginx/modules/strm_packager/src/common/source.h>
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>
#include <nginx/modules/strm_packager/src/content/description.h>

namespace NStrm::NPackager {
    class TVodWorker: public TRequestWorker {
    public:
        TVodWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

    private:
        void Work() override;

        TString GetDescriptionUri(TString uri) const;
        TString GetContentUri(TString uri) const;
        bool IsCmafEnabled(EContainer container) const;
        Ti64TimeMs GetChunkDuration(const NDescription::TDescription* description) const;
        TVector<TSourceFuture> MakeSources(const TVector<NDescription::TSourceInfo>& sourceInfos,
                                           const TSourceMp4File::TConfig& mp4SourceConfig);

        TMaybe<TIntervalP> SegmentInterval;
    };
}
