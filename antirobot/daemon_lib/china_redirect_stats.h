#pragma once

#include <antirobot/lib/stats_writer.h>

#include <library/cpp/deprecated/atomic/atomic.h>

namespace NAntiRobot {
    struct TChinaRedirectStats {
        TAtomic RequestsFromChinaNotLoginned = 0;
        TAtomic RedirectedToLoginRequestsFromChina = 0;
        TAtomic UnauthorizedRequestsFromChina = 0;

        TAtomic IsChinaRedirectEnabled = 0;
        TAtomic IsChinaUnauthorizedEnabled = 0;

        void PrintStats(TStatsWriter& out) const {
            out.WriteScalar("requests_from_china_not_loginned", AtomicGet(RequestsFromChinaNotLoginned));
            out.WriteScalar("redirected_to_login_requests_from_china", AtomicGet(RedirectedToLoginRequestsFromChina));
            out.WriteScalar("unauthorized_request_from_china", AtomicGet(UnauthorizedRequestsFromChina));

            out.WriteHistogram("is_china_redirect_enabled", AtomicGet(IsChinaRedirectEnabled));
            out.WriteHistogram("is_china_unauthorized_enabled", AtomicGet(IsChinaUnauthorizedEnabled));
        }
    };
}
