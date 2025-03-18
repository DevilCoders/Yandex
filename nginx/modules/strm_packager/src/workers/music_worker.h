#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>
#include <nginx/modules/strm_packager/src/common/source.h>
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>
#include <nginx/modules/strm_packager/src/content/description.h>
#include <nginx/modules/strm_packager/src/content/music_uri.h>

namespace NStrm::NPackager {
    class TMusicWorker: public TRequestWorker {
    public:
        TMusicWorker(TRequestContext& context, const TLocationConfig& config);

        static void CheckConfig(const TLocationConfig& config);

    private:
        void Work() override;
        void WorkCaf(const NThreading::TFuture<TString>& sourcePath, const TString& key);
        void WorkMp4(const NThreading::TFuture<TString>& sourcePath, const TString& key);
        TString GetContentUri(const TString& uri) const;
        TString GetMetaUri(const TString& uri) const;

        const TString iv;
    };
}
