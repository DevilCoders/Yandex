#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/source.h>
#include <nginx/modules/strm_packager/src/common/source_file.h>

namespace NStrm::NPackager {
    class TSourceMp4File: public TSourceFile {
    public:
        struct TConfig: public TSourceFile::TConfig {
            TConfig();
            explicit TConfig(const TLocationConfig& locationConfig);

            void Check() const;

            TMaybe<ui64> MoovScanBlockSize;
        };

        static TSourceFuture Make(
            TRequestWorker& request,
            const TConfig& config,
            const TString& uri,
            const TString& args,
            const TMaybe<TIntervalP> cropInterval = {},
            const TMaybe<Ti64TimeP> offset = {},
            const bool getWholeFile = false);

    private:
        TSourceMp4File() = delete;
    };

}
