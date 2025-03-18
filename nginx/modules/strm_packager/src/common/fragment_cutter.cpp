#include <nginx/modules/strm_packager/src/common/fragment_cutter.h>

namespace NStrm::NPackager {
    // static
    TRepFuture<TMediaData> TFragmentCutter::Cut(TRepFuture<TMediaData> dataFuture, Ti64TimeP fragmentDuration, TRequestWorker& request) {
        return request.GetPoolUtil<TFragmentCutter>().New(dataFuture, fragmentDuration, request)->GetData();
    }

    TFragmentCutter::TFragmentCutter(TRepFuture<TMediaData> dataFuture, Ti64TimeP fragmentDuration, TRequestWorker& request)
        : Request(request)
        , FragmentDuration(fragmentDuration)
        , SendOnce(false)
        , DataPromise(TRepPromise<TMediaData>::Make())
    {
        dataFuture.AddCallback(request.MakeFatalOnException(std::bind(&TFragmentCutter::AcceptData, this, std::placeholders::_1)));
    }

    TRepFuture<TMediaData> TFragmentCutter::GetData() const {
        return DataPromise.GetFuture();
    }

    void TFragmentCutter::AcceptData(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (dataRef.Empty()) {
            SendReadyFragments(true);

            Y_ENSURE(Acc.Interval.Begin == Acc.Interval.End);

            if (FullInterval.Defined() && !SendOnce) {
                // got no samples, skipped every interval as empty, so have to put single empty interval
                Acc.Interval = *FullInterval;
                DataPromise.PutData(std::move(Acc));
            }
            DataPromise.Finish();
            return;
        }

        const TMediaData& data = dataRef.Data();
        Y_ENSURE(data.TracksSamples.size() == data.TracksInfo.size());

        if (FullInterval.Empty()) {
            FullInterval = data.Interval;
        } else {
            FullInterval->End = data.Interval.End;
        }

        // accumulate data
        if (Acc.TracksInfo.empty()) {
            Acc = data;
        } else {
            Y_ENSURE(Acc.TracksInfo.size() == data.TracksInfo.size());
            Y_ENSURE(data.Interval.Begin >= Acc.Interval.End);

            Acc.Interval.End = data.Interval.End;
            for (size_t trackIndex = 0; trackIndex < data.TracksSamples.size(); ++trackIndex) {
                const TVector<TSampleData>& src = data.TracksSamples[trackIndex];
                TVector<TSampleData>& dst = Acc.TracksSamples[trackIndex];

                dst.insert(dst.end(), src.begin(), src.end());
            }
        }

        SendReadyFragments(false);
    }

    void TFragmentCutter::SendReadyFragments(bool final) {
        TVector<TVector<TSampleData>::iterator> TrackSamplesBegin;

        const size_t tracksCount = Acc.TracksSamples.size();

        while (final ? (Acc.Interval.Begin < Acc.Interval.End) : (Acc.Interval.Begin + FragmentDuration <= Acc.Interval.End)) {
            if (TrackSamplesBegin.empty()) {
                TrackSamplesBegin.resize(tracksCount);
                for (size_t trackIndex = 0; trackIndex < tracksCount; ++trackIndex) {
                    TrackSamplesBegin[trackIndex] = Acc.TracksSamples[trackIndex].begin();
                }
            }

            TMediaData result;
            result.Interval.Begin = Acc.Interval.Begin;
            result.Interval.End = Min(Acc.Interval.Begin + FragmentDuration, Acc.Interval.End);
            result.TracksInfo = Acc.TracksInfo;
            result.TracksSamples.resize(tracksCount);

            bool empty = true;

            for (size_t trackIndex = 0; trackIndex < tracksCount; ++trackIndex) {
                auto& begin = TrackSamplesBegin[trackIndex];
                const auto range = GetSamplesInterval(begin, Acc.TracksSamples[trackIndex].end(), result.Interval, Request.KalturaMode);
                if (!range.Begin) {
                    continue;
                }

                const auto end = begin + (range.End - &*begin);

                empty = empty && (begin == end);

                result.TracksSamples[trackIndex].assign(begin, end);

                begin = end;
            }

            Acc.Interval.Begin = result.Interval.End;

            if (empty && SkipEmpty) {
                // skip
            } else {
                SendOnce = true;
                DataPromise.PutData(std::move(result));
            }
        }

        if (!TrackSamplesBegin.empty()) {
            for (size_t trackIndex = 0; trackIndex < tracksCount; ++trackIndex) {
                Acc.TracksSamples[trackIndex].erase(Acc.TracksSamples[trackIndex].begin(), TrackSamplesBegin[trackIndex]);
            }
        }
    }

}
