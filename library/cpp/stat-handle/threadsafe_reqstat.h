#pragma once

#include "reqstat.h"

namespace NStat {
    /*! @brief Basic statistic gathering class shared by various components, currently:
 * - wizard
 * - begemot
 * - lingvo booster
 *
 * The class can report statistics in a thread-safe manner
 * using mutex-guarded data access.
 */
    class TThreadSafeReqStat: public TRequestStat {
    public:
        // by default, request statistics is kept for the last 24 hours
        TThreadSafeReqStat(const TDuration& collectionSpan = TDuration::Hours(24),
                           TVector<TDurationHistogram::TColumnGroup>&& groups = {
                               // bars up to 100 ms are 1 ms-width
                               {TDuration::MilliSeconds(100), TDuration::MilliSeconds(1)},
                               // then bars up to 1 s are 5 ms-width
                               {TDuration::MilliSeconds(1000), TDuration::MilliSeconds(5)},
                               // ... hope you caught the pattern
                               {TDuration::MilliSeconds(10000), TDuration::MilliSeconds(10)}})
            : TRequestStat(collectionSpan)
        {
            InitHistogram(std::move(groups));
        }
        //! Generate the histogram. 1 (integer) in the response time field equals 1 @c timeUnit.
        NSc::TValue HistogramJson(TDuration timeUnit = TDuration::MicroSeconds(1)) const {
            TGuard<TMutex> guard(Mutex);
            return TRequestStat::HistogramJson(timeUnit);
        }

        //! Dump all the statistics as a protobuf.
        //! @see library/cpp/stat-handle/proto/stat.proto
        void ToProto(TStatProto& proto) const override {
            TGuard<TMutex> guard(Mutex);
            TRequestStat::ToProto(proto);
        }
        using TStat::ToProto;
        //! Dump the protobuf of this object as json
        TString ToJson() const override;

        //! Here come stats about all the requests.
        void RegisterRequest(const NStat::TTimeInfo& timing, const TString& clientId) override {
            TGuard<TMutex> guard(Mutex);
            TRequestStat::RegisterRequest(timing, clientId);
        }

        //! Here come errors for requests where clientId can not be detected, and processing has not been started.
        void RegisterError(const TStringBuf& type) override {
            TGuard<TMutex> guard(Mutex);
            TRequestStat::RegisterError(type);
        }

    protected:
        // Let's make all Print* methods thread-safe also
        TString DoPrint(const TString& title, size_t top, bool init, bool runtime) const override {
            TGuard<TMutex> guard(Mutex);
            return TRequestStat::DoPrint(title, top, init, runtime);
        }

        TMutex Mutex;

        using TStat::GetOrMakeRecord;
    };
}
