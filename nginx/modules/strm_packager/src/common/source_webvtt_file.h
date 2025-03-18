#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/source.h>

namespace NStrm::NPackager {
    class TSourceWebvttFile: public ISource {
    public:
        static TSourceFuture Make(
            TRequestWorker& request,
            const TString& uri,
            const TString& args,
            const TIntervalP cropInterval = {},
            const TMaybe<Ti64TimeP> offset = {});

        TSourceWebvttFile(
            TRequestWorker& request,
            TStringBuf fileData,
            const TIntervalP cropInterval,
            const Ti64TimeP offset);

    private:
        TIntervalP FullInterval() const override;
        TVector<TTrackInfo const*> GetTracksInfo() const override;
        TRepFuture<TMediaData> GetMedia() override;

        const TIntervalP Interval;
        const TVector<TTrackInfo const*> Info;
        TRepPromise<TMediaData> Data;
    };

}
