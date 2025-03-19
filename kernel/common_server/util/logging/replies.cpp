#include "replies.h"

using namespace NJson;
using namespace NUtil;

void TRepliesStorage::SetCapacity(ui64 capacity) {
    TGuard<TMutex> guard(Lock);
    Capacity = capacity;

    if (!Capacity)
        ServiceRecords.clear();
}

void TRepliesStorage::AddDocument(const TString& service, TRecordPtr record) {
    if (!Capacity || !record)
        return;

    TGuard<TMutex> guard(Lock);
    TRecords& records = ServiceRecords[service];

    records.push_front(record);
    if (Y_LIKELY(records.size() > Capacity))
        records.pop_back();
}

NJson::TJsonValue TRepliesStorage::GetReport() {
    if (!Capacity)
        return TString("no replies");

    NJson::TJsonValue result;
    {
        TGuard<TMutex> guard(Lock);
        DoReport(ServiceRecords, result);
    }
    return result;
}

void TRepliesStorage::DoReport(const TServiceRecords& records, NJson::TJsonValue& result) const {
    for (auto&& service : records) {
        TJsonValue serviceRecord;
        for (auto&& record : service.second) {
            TJsonValue jsonRecord(JSON_MAP);
            jsonRecord.InsertValue("reply", record->Reply);
            jsonRecord.InsertValue("description", record->Document);
            jsonRecord.InsertValue("time_processed", record->TimeProcessed);
            jsonRecord.InsertValue("timestamp", record->Timestamp);
            serviceRecord.AppendValue(jsonRecord);
        }
        result.InsertValue(service.first, serviceRecord);
    }
}
