#include <nginx/modules/strm_packager/src/common/track_data.h>
#include <nginx/modules/strm_packager/src/base/workers.h>

#include <util/generic/deque.h>

namespace NStrm::NPackager {
    class TRequireDataState {
    public:
        TRequireDataState(const TRepFuture<TMediaData>& data, TRequestWorker& request);

        TRepFuture<TMediaData> GetFuture() const;

    private:
        void MediaDataCallback(const TRepFuture<TMediaData>::TDataRef& data);
        void SampleDataCallback(const NThreading::TFuture<void>& sampleDataFuture);
        void TrySend();

    private:
        TRequestWorker& Request;
        TRepPromise<TMediaData> Promise;
        bool Error;
        bool InputFinished;
        bool PromiseFinished;
        TDeque<std::pair<NThreading::TFuture<void>, TMediaData>> Acc;
    };

    TRequireDataState::TRequireDataState(const TRepFuture<TMediaData>& data, TRequestWorker& request)
        : Request(request)
        , Promise(TRepPromise<TMediaData>::Make())
        , Error(false)
        , InputFinished(false)
        , PromiseFinished(false)
    {
        data.AddCallback(request.MakeFatalOnException(std::bind(&TRequireDataState::MediaDataCallback, this, std::placeholders::_1)));
    }

    void TRequireDataState::MediaDataCallback(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (Error) {
            return;
        }

        if (dataRef.Exception()) {
            Error = true;
            Promise.FinishWithException(dataRef.Exception());
            return;
        }

        if (dataRef.Empty()) {
            InputFinished = true;
            TrySend();
            return;
        }

        const TMediaData& data = dataRef.Data();

        TVector<NThreading::TFuture<void>> toWait;

        for (const auto& track : data.TracksSamples) {
            for (const TSampleData& sample : track) {
                const NThreading::TFuture<void>& f = sample.DataFuture;
                Y_ENSURE(f.Initialized());

                if (toWait.empty() || toWait.back().StateId() != f.StateId()) {
                    toWait.push_back(f);
                }
            }
        }

        Acc.emplace_back(PackagerWaitExceptionOrAll(toWait), data);

        Acc.back().first.Subscribe(Request.MakeFatalOnException(std::bind(&TRequireDataState::SampleDataCallback, this, std::placeholders::_1)));
    }

    void TRequireDataState::SampleDataCallback(const NThreading::TFuture<void>& future) {
        try {
            future.TryRethrow();
        } catch (const TFatalExceptionContainer&) {
            throw;
        } catch (...) {
            Request.LogException("TRequireDataState::SampleDataCallback got future exception:", std::current_exception());
            if (Error) {
                return;
            }
            Error = true;
            Promise.FinishWithException(std::current_exception());
            return;
        }

        TrySend();
    }

    void TRequireDataState::TrySend() {
        if (Error) {
            return;
        }

        while (!Acc.empty() && Acc.front().first.HasValue()) {
            Y_ENSURE(!PromiseFinished);
            TMediaData data = std::move(Acc.front().second);
            Acc.pop_front();
            Promise.PutData(std::move(data));
        }

        if (Acc.empty() && InputFinished && !PromiseFinished) {
            PromiseFinished = true;
            Promise.Finish();
        }
    }

    TRepFuture<TMediaData> TRequireDataState::GetFuture() const {
        return Promise.GetFuture();
    }

    TRepFuture<TMediaData> RequireData(const TRepFuture<TMediaData>& data, TRequestWorker& request) {
        return request.GetPoolUtil<TRequireDataState>().New(data, request)->GetFuture();
    }

    TRepFuture<TMetaData> MakeSimpleMeta(const TIntervalP interval, const TMaybe<TMetaData::TAdData> ad) {
        Y_ENSURE(interval.Begin < interval.End);

        TRepPromise<TMetaData> promise = TRepPromise<TMetaData>::Make();

        if (ad.Defined()) {
            const TIntervalP adBlockInterval{
                .Begin = ad->BlockBegin,
                .End = ad->BlockEnd.GetOrElse(Max(interval.End, ad->BlockBegin)),
            };

            const TIntervalP intersection{
                .Begin = Max(interval.Begin, adBlockInterval.Begin),
                .End = Min(interval.End, adBlockInterval.End),
            };

            if (intersection.Begin < intersection.End) {
                if (interval.Begin < intersection.Begin) {
                    promise.PutData(TMetaData{
                        .Interval = TIntervalP{
                            .Begin = interval.Begin,
                            .End = intersection.Begin,
                        },
                        .Data = TMetaData::TSourceData(),
                    });
                }

                promise.PutData(TMetaData{
                    .Interval = intersection,
                    .Data = *ad,
                });

                if (intersection.End < interval.End) {
                    promise.PutData(TMetaData{
                        .Interval = TIntervalP{
                            .Begin = intersection.End,
                            .End = interval.End,
                        },
                        .Data = TMetaData::TSourceData(),
                    });
                }

                promise.Finish();
                return promise.GetFuture();
            }
        }

        promise.PutData(TMetaData{
            .Interval = interval,
            .Data = TMetaData::TSourceData(),
        });

        promise.Finish();
        return promise.GetFuture();
    }

}
