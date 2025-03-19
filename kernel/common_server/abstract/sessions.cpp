#include "sessions.h"

NJson::TJsonValue ISession::GetReportMeta() const {
    NJson::TJsonValue result;
    if (GetSessionId()) {
        result.InsertValue("session_id", GetSessionId());
    }
    result.InsertValue("object_id", ObjectId);
    result.InsertValue("user_id", UserId);
    result.InsertValue("start", StartTS.Seconds());
    if (GetClosed()) {
        result.InsertValue("finish", LastTS.Seconds());
    } else {
        result.InsertValue("current", LastTS.Seconds());
    }
    result.InsertValue("finished", GetClosed());
    result.InsertValue("corrupted", CurrentState == ECurrentState::Corrupted);
    return result;
}

bool ISession::OccuredInInterval(const TInstant since, const TInstant until) const {
    const TInstant minMax = Min(until, LastTS);
    const TInstant maxMin = Min(since, StartTS);
    return (minMax > maxMin);
}
