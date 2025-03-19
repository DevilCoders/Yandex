#include "rate.h"

#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/logger/global/global.h>

#include <util/random/random.h>

#include <cmath>

TSimpleReaskLimiter::~TSimpleReaskLimiter() {
    CHECK_WITH_LOG(AtomicGet(ReasksCount) == 0) << AtomicGet(ReasksCount) << Endl;
    CHECK_WITH_LOG(AtomicGet(RequestsCount) == 0) << AtomicGet(RequestsCount) << Endl;
}

TSimpleReaskLimiter::TSimpleReaskLimiter(const float rate)
    : Rate(rate)
{
    CHECK_WITH_LOG(Rate > 0);
}

NJson::TJsonValue TSimpleReaskLimiter::GetReport() const {
    NJson::TJsonValue result;
    result["req_count"] = AtomicGet(RequestsCount);
    result["ra_count"] = AtomicGet(ReasksCount);
    return result;
}

void TSimpleReaskLimiter::OnRequest(const bool isReask) {
    if (isReask) {
        AtomicIncrement(ReasksCount);
    } else {
        AtomicIncrement(RequestsCount);
    }
    DEBUG_LOG << AtomicGet(ReasksCount) << " / " << AtomicGet(RequestsCount) << Endl;
}

void TSimpleReaskLimiter::OnReply(const bool isReask) {
    if (isReask) {
        CHECK_WITH_LOG(AtomicDecrement(ReasksCount) >= 0);
    } else {
        CHECK_WITH_LOG(AtomicDecrement(RequestsCount) >= 0);
    }
    DEBUG_LOG << AtomicGet(ReasksCount) << " / " << AtomicGet(RequestsCount) << Endl;
}

bool TSimpleReaskLimiter::CanReask() const {
    i64 rCount = AtomicGet(RequestsCount);
    if (!rCount) {
        return true;
    }
    i64 reaskCount = AtomicGet(ReasksCount);
    DEBUG_LOG << reaskCount << " / " << rCount << Endl;
    return (1.0 * reaskCount / rCount) < Rate;
}
