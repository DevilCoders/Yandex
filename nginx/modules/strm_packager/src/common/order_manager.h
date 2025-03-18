#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>
#include <nginx/modules/strm_packager/src/common/track_data.h>

#include <util/generic/queue.h>

namespace NStrm::NPackager {
    class TOrderManager {
    public:
        TOrderManager(); // construct without request in unit tests only

        TOrderManager(TRequestWorker& request);

        // order of calls SetTimeBegin, SetTimeEnd and PutData is not restricted
        //  the only restriction - data in PutData must fully cover
        //  interval [timeBegin; timeEnd) without intersections
        void SetTimeBegin(Ti64TimeP timeBegin);           // call once
        void SetTimeEnd(Ti64TimeP timeEnd);               // call once
        void PutData(const TRepFuture<TMediaData>& data); // call many times

        TMaybe<Ti64TimeP> GetTimeBegin() const;
        TMaybe<Ti64TimeP> GetTimeEnd() const;

        TRepFuture<TMediaData> GetData();

    private:
        TOrderManager(TRepFuture<TMediaData>::TCallback acceptMediaDataBind);

    private:
        void AcceptMediaData(const TRepFuture<TMediaData>::TDataRef&);

        void UpdateData();

        static bool Compare(const TMediaData&, const TMediaData&);

        TRepFuture<TMediaData>::TCallback AcceptMediaDataBind;

        TMaybe<Ti64TimeP> TimeBegin;
        TMaybe<Ti64TimeP> TimeEnd;
        TMaybe<Ti64TimeP> SentTimeEnd;

        TPriorityQueue<TMediaData, TVector<TMediaData>, bool (*)(const TMediaData&, const TMediaData&)> PendingData;

        TRepPromise<TMediaData> DataPromise;
    };

    // this constructor made inline since otherwise unit test will have linker error `duplicate symbol: main` since it will link with nginx
    inline TOrderManager::TOrderManager(TRequestWorker& request)
        : TOrderManager(request.MakeFatalOnException(std::bind(&TOrderManager::AcceptMediaData, this, std::placeholders::_1)))
    {
    }

    // this constructor is inline just to be near the other
    inline TOrderManager::TOrderManager()
        : TOrderManager(std::bind(&TOrderManager::AcceptMediaData, this, std::placeholders::_1))
    {
    }
}
