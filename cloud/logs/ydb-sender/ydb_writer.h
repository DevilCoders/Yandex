#pragma once

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/system/mutex.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

class TYdbWriter {
public:
    struct TColInfo {
        TString Name;
        TString ParamName;
        NYdb::EPrimitiveType Type;
    };

    struct TSchema {
        THashMap<TString, TColInfo> Columns;
        TVector<TString> KeyColumns;
    };

public:
    TYdbWriter(const NYdb::TDriver& driver, TString tablePath, TString tableProfile, ui64 sessionLimit, TSchema&& schema);
    NYdb::TAsyncStatus Write(TString table, const TVector<NYdb::TValue>& batch);
    NYdb::TAsyncStatus WriteStreams(const TVector<NYdb::TValue> streams);

private:
    TString TouchTable(TString name);
    void TouchStreamsTable();
    void DoCreateTable(TString name);
    void DoCreateStreamsTable();

private:
    struct TTableInfo {
        TString Query;
        TInstant LastAccess;
    };

    TSchema Schema;
    TString TablePath;
    TString TableProfileName;
    TString QueryTemplate;
    TString StreamsQuery;
    TAtomic StreamsTableCreated = 0;
    NYdb::NTable::TTableClient Client;

    TAtomic QueriesInFlight = 0;


    TMutex KnownTablesMutex;
    THashMap<TString, TTableInfo> KnownTables;
};
