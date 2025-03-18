#pragma once

#include "time_stats.h"

namespace NAntiRobot {

    struct TForwardingStats {
        TTimeStats ForwardRequestTimeSingle = {TIME_STATS_VEC, "forward_request_single_"};
        TTimeStats ForwardRequestTimeWithRetries = {TIME_STATS_VEC, "forward_request_with_retries_"};
        TAtomic ForwardRequestExceptions = 0;
        TAtomic ForwardRequestsAll = 0;
        TAtomic ForwardRequestsFrom1stTry = 0;
        TAtomic ForwardRequestsFrom2ndTry = 0;
        TAtomic ForwardRequestsFrom3rdTry = 0;

        void PrintStats(TStatsWriter& out) const {
            ForwardRequestTimeSingle.PrintStats(out);
            ForwardRequestTimeWithRetries.PrintStats(out);

            out.WriteScalar("forward_request_exceptions", AtomicGet(ForwardRequestExceptions));
            out.WriteScalar("forward_request_all", AtomicGet(ForwardRequestsAll));
            out.WriteScalar("forward_request_1st_try", AtomicGet(ForwardRequestsFrom1stTry));
            out.WriteScalar("forward_request_2nd_try", AtomicGet(ForwardRequestsFrom2ndTry));
            out.WriteScalar("forward_request_3rd_try", AtomicGet(ForwardRequestsFrom3rdTry));
        }
    };

}
