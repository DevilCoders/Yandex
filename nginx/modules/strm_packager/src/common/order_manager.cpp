#include <nginx/modules/strm_packager/src/common/order_manager.h>
#include <library/cpp/threading/future/future.h>

namespace NStrm::NPackager {
    TOrderManager::TOrderManager(TRepFuture<TMediaData>::TCallback acceptMediaDataBind)
        : AcceptMediaDataBind(acceptMediaDataBind)
        , PendingData(&TOrderManager::Compare)
        , DataPromise(TRepPromise<TMediaData>::Make())
    {
    }

    // comparator for TPriorityQueue that will have top data with minimal Begin
    bool TOrderManager::Compare(const TMediaData& a, const TMediaData& b) {
        return a.Interval.Begin > b.Interval.Begin;
    }

    void TOrderManager::SetTimeBegin(Ti64TimeP timeBegin) {
        Y_ENSURE(TimeBegin.Empty());
        TimeBegin = timeBegin;
        Y_ENSURE(TimeEnd.Empty() || *TimeBegin < *TimeEnd);
        SentTimeEnd = TimeBegin;

        UpdateData();
    }

    void TOrderManager::SetTimeEnd(Ti64TimeP timeEnd) {
        Y_ENSURE(TimeEnd.Empty());
        TimeEnd = timeEnd;
        Y_ENSURE(TimeBegin.Empty() || *TimeBegin < *TimeEnd);

        UpdateData();
    }

    void TOrderManager::PutData(const TRepFuture<TMediaData>& data) {
        data.AddCallback(AcceptMediaDataBind);
    }

    TMaybe<Ti64TimeP> TOrderManager::GetTimeBegin() const {
        return TimeBegin;
    }

    TMaybe<Ti64TimeP> TOrderManager::GetTimeEnd() const {
        return TimeEnd;
    }

    TRepFuture<TMediaData> TOrderManager::GetData() {
        return DataPromise.GetFuture();
    }

    void TOrderManager::AcceptMediaData(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (dataRef.Empty()) {
            return;
        }

        const TMediaData& data = dataRef.Data();

        PendingData.push(data);

        UpdateData();
    }

    void TOrderManager::UpdateData() {
        if (SentTimeEnd.Empty()) {
            return;
        }

        while (!PendingData.empty() && PendingData.top().Interval.Begin == *SentTimeEnd) {
            TMediaData data(std::move(PendingData.top()));
            PendingData.pop();
            SentTimeEnd = data.Interval.End;
            DataPromise.PutData(std::move(data));
        }

        if (TimeEnd.Empty()) {
            return;
        }

        Y_ENSURE(*SentTimeEnd <= *TimeEnd);

        if (*SentTimeEnd == *TimeEnd) {
            Y_ENSURE(PendingData.empty());
            DataPromise.Finish();
        }
    }

}
