#pragma once

#include "stat.h"

#include <atomic>


namespace NAntiRobot {


enum class EServerExceptionCounter {
    BadRequests             /* "bad_requests_count" */,
    BadExpects              /* "bad_expects_count" */,
    InvalidPartnerRequests  /* "invalid_partner_requests_count" */,
    UidCreationFailures     /* "uid_creation_failures_count" */,
    OtherExceptions         /* "other_exceptions_count" */,
    Count
};

using TServerExceptionsStats = TCategorizedStats<
    std::atomic<size_t>, EServerExceptionCounter
>;


} // namespace NAntiRobot
