#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/muxer.h>

#include <util/generic/deque.h>

namespace NStrm::NPackager {
    class TMuxerWebvtt: public IMuxer {
    public:
        TMuxerWebvtt(
            TRequestWorker& request);

        static TMuxerFuture Make(
            TRequestWorker& request);

        TSimpleBlob MakeInitialSegment(const TVector<TTrackInfo const*>& tracksInfo) const override;
        void SetMediaData(TRepFuture<TMediaData> data) override;
        TRepFuture<TSendData> GetData() override;
        TString GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const override;

    private:
        void MediaDataCallback(const TRepFuture<TMediaData>::TDataRef& dataRef);

    private:
        TRequestWorker& Request;

        TRepPromise<TSendData> DataPromise;
    };
}
