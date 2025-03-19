#include "sequential.h"

TInstant IBaseSequentialTableImpl::GetInstantByHistoryEventId(const i64 historyEventId, NStorage::ITransaction::TPtr transaction) const {
    TStringBuilder request;
    request << "SELECT " << GetTimestampFieldName() << " as ts FROM " << GetTableName() << " WHERE " << GetSeqFieldName() << " = " << historyEventId;
    TRecordsSetWT records;
    auto reqResult = transaction->Exec(request, &records);
    CHECK_WITH_LOG(reqResult->IsSucceed()) << transaction->GetStringReport() << Endl;
    CHECK_WITH_LOG(records.size() == 1) << records.size() << Endl;
    const i64 result = records.front().TryGetDefault<i64>("ts", -1);
    CHECK_WITH_LOG(result != -1);
    return TInstant::Seconds(result);
}

i64 IBaseSequentialTableImpl::GetSuppEventIdNearest(const i64 historyEventId, const i64 min, NStorage::ITransaction::TPtr transaction) const {
    TStringBuilder request;
    request << "SELECT MAX(" << GetSeqFieldName() << ") as id FROM " << GetTableName() << " WHERE " << GetSeqFieldName() << " <= " << historyEventId << " AND " << GetSeqFieldName() << " >= " << min;
    TRecordsSetWT records;
    auto reqResult = transaction->Exec(request, &records);
    CHECK_WITH_LOG(reqResult->IsSucceed()) << transaction->GetStringReport() << Endl;
    CHECK_WITH_LOG(records.size() == 1) << records.size() << Endl;
    const i64 result = records.front().TryGetDefault<i64>("id", -1);
    CHECK_WITH_LOG(result != -1);
    return result;
}

bool IBaseSequentialTableImpl::GetNextInterval(const i64 min, const i64 max, const TInstant instant, i64& minNext, i64& maxNext, NStorage::ITransaction::TPtr transaction) const {
    const i64 currentId = 0.5 * (max + min);
    if (currentId == max || currentId == min) {
        minNext = min;
        maxNext = min;
        return false;
    }

    const i64 realCurrentId = GetSuppEventIdNearest(currentId, min, transaction);
    const TInstant currentInstant = GetInstantByHistoryEventId(realCurrentId, transaction);
    if (currentInstant > instant) {
        minNext = min;
        maxNext = realCurrentId;
    } else if (currentInstant < instant) {
        minNext = realCurrentId;
        maxNext = max;
    } else {
        minNext = realCurrentId;
        maxNext = realCurrentId;
        return false;
    }
    if (minNext == min && maxNext == max) {
        minNext = min;
        maxNext = min;
        return false;
    }
    return true;
}

i64 IBaseSequentialTableImpl::GetHistoryEventIdByTimestamp(const TInstant instant, const TString& info) const {
    TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> gTime("history event by instant for " + info + " in " + GetTableName());
    if (!GetTimestampFieldName()) {
        return 0;
    }
    auto transaction = Database->CreateTransaction(true);
    i64 maxEventId = -1;
    i64 minEventId = -1;

    {
        TStringBuilder request;
        request << "SELECT MAX(" << GetSeqFieldName() << ") as max_history_event_id FROM " << GetTableName();
        TRecordsSetWT records;
        auto reqResult = transaction->Exec(request, &records);
        CHECK_WITH_LOG(reqResult->IsSucceed()) << transaction->GetStringReport() << Endl;
        CHECK_WITH_LOG(records.size() == 1) << records.size() << Endl;
        maxEventId = records.front().TryGetDefault<i64>("max_history_event_id", -1);
        if (maxEventId == -1) {
            return -1;
        }
    }

    {
        TStringBuilder request;
        request << "SELECT MIN(" << GetSeqFieldName() << ") as min_history_event_id FROM " << GetTableName();
        TRecordsSetWT records;
        auto reqResult = transaction->Exec(request, &records);
        CHECK_WITH_LOG(reqResult->IsSucceed()) << transaction->GetStringReport() << Endl;
        CHECK_WITH_LOG(records.size() == 1) << records.size() << Endl;
        minEventId = records.front().TryGetDefault<i64>("min_history_event_id", -1);
        CHECK_WITH_LOG(minEventId != -1) << Endl;
    }

    const TInstant minInstant = GetInstantByHistoryEventId(minEventId, transaction);
    const TInstant maxInstant = GetInstantByHistoryEventId(maxEventId, transaction);
    if (minInstant >= instant) {
        return minEventId;
    }
    if (maxInstant <= instant) {
        return maxEventId;
    }
    i64 nextMinEventId = -1;
    i64 nextMaxEventId = -1;
    while (GetNextInterval(minEventId, maxEventId, instant, nextMinEventId, nextMaxEventId, transaction)) {
        minEventId = nextMinEventId;
        maxEventId = nextMaxEventId;
    }
    CHECK_WITH_LOG(nextMinEventId == nextMaxEventId) << nextMaxEventId << "/" << nextMinEventId << Endl;
    return nextMinEventId;
}

TMap<TDuration, TString> THistoryConditionsTimeline::BuildFinalConditions() {
    TMap<TDuration, TString> result;
    TMap<TString, TSet<TString>> fullCondition;

    for (auto[dt, conditions] : Reversed(Conditions)) {
        for (auto&& i : conditions) {
            fullCondition[i.first].insert(i.second.begin(), i.second.end());
        }
        TStringStream ss;
        for (auto&& i : fullCondition) {
            if (!ss.Empty()) {
                ss << " OR ";
            }
            ss << i.first << " IN ('" << JoinSeq("','", i.second) << "')";
        }
        result[dt] = ss.Str();
    }
    return result;
}
