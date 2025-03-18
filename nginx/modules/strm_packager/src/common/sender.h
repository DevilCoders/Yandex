#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/drm_info_util.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>

#include <library/cpp/threading/future/future.h>

namespace NStrm::NPackager {
    class ISender {
    public:
        virtual ~ISender() = default;
        virtual void SetData(const TRepFuture<TSendData>& data) = 0;

    protected:
        ISender() = default;
    };

    using TSenderFuture = NThreading::TFuture<ISender*>;
    using TSenderPromise = NThreading::TPromise<ISender*>;

    // base implementation
    class TSender: public ISender {
    public:
        TSender(TRequestWorker&);

        static TSenderFuture Make(TRequestWorker&);
        static TSenderFuture Make(TRequestWorker&, const NThreading::TFuture<TMaybe<TDrmInfo>>& drmFuture);

        void SetData(const TRepFuture<TSendData>& data) final;

    private:
        virtual void Send(const TSendData& data);
        virtual void Finish();

    protected:
        TRequestWorker& Request;

    private:
        bool IsDataSet;
    };

}
