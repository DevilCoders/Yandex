#pragma once

#include "metrics.h"

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/hash.h>

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

class TYDBExecuter
{
private:
    TString Database;
    NYdb::TDriver YdbDriver;
    NYdb::NTable::TTableClient YdbClient;

public:
    struct TRawMetricsData
    {
        TString loadData;
        ui64 timestamp = 0;
    };

    using TRawData = THashMap<TString, TVector<TRawMetricsData>>;

    TYDBExecuter(
        const TString& endpoint,
        TString database,
        const TString& token);

    void Stop();

    TRawData GetData(
        const TString& table,
        TDuration timestampFrom,
        TDuration timestampTo);

    TRawData GetAllData(
        const TString& table,
        TDuration timestampFrom,
        TDuration timestampTo);
};

}   // NCloud::NBlockStore::NAnalyzeUsedGroup
