#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/track_data.h>

namespace NStrm::NPackager {
    class TFragmentCutter {
    public:
        static TRepFuture<TMediaData> Cut(TRepFuture<TMediaData> dataFuture, Ti64TimeP FragmentDuration, TRequestWorker& request);

        TFragmentCutter(TRepFuture<TMediaData> dataFuture, Ti64TimeP FragmentDuration, TRequestWorker& request);

        TRepFuture<TMediaData> GetData() const;

    private:
        void AcceptData(const TRepFuture<TMediaData>::TDataRef& dataRef);

        void SendReadyFragments(bool final);

        const TRequestWorker& Request;

        const Ti64TimeP FragmentDuration;

        // maybe get this from arguments, but right now looks like everyone need SkipEmpty == true
        static constexpr bool SkipEmpty = true;

        bool SendOnce;
        TMaybe<TIntervalP> FullInterval;

        TRepPromise<TMediaData> DataPromise;

        TMediaData Acc;
    };
}
