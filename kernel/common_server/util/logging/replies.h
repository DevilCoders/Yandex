#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/list.h>
#include <util/generic/map.h>
#include <util/system/mutex.h>

namespace NUtil {

class TRepliesStorage {
public:
    struct TRecord {
        TString Reply;
        TString Document;
        ui64 TimeProcessed = 0;
        ui64 Timestamp = 0;
    };
    typedef TSimpleSharedPtr<TRecord> TRecordPtr;
    typedef TList<TRecordPtr> TRecords;
    typedef TMap<TString, TRecords> TServiceRecords;

public:
    TRepliesStorage(ui64 capacity = 0)
        : Capacity(capacity)
    {}

    void AddDocument(const TString& service, TRecordPtr record);
    void SetCapacity(ui64 capacity);
    NJson::TJsonValue GetReport();

private:
    void DoReport(const TServiceRecords& records, NJson::TJsonValue& result) const;
private:
    ui64 Capacity;

    TServiceRecords ServiceRecords;
    TMutex Lock;
};
}
