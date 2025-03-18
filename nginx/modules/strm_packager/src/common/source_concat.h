#pragma once

#include <nginx/modules/strm_packager/src/common/source.h>

namespace NStrm::NPackager {
    class TSourceConcat: public ISource {
    public:
        static TSourceFuture Make(
            TRequestWorker& request,
            const TVector<TSourceFuture>& sources);

        TSourceConcat(
            TRequestWorker& request,
            const TVector<TSourceFuture>& sources);

    public:
        TIntervalP FullInterval() const override;
        TVector<TTrackInfo const*> GetTracksInfo() const override;
        TRepFuture<TMediaData> GetMedia() override;
        TRepFuture<TMediaData> GetMedia(TIntervalP interval, Ti64TimeP addTsOffset) override;

    private:
        TIntervalP Interval;
        TVector<ISource*> Sources;
    };
}
