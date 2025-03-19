#pragma once

#include "public.h"

#include "ydbscheme.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/kikimr/events.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/startable.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>
#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/datetime/base.h>

#include <utility>

namespace NCloud::NBlockStore::NYdbStats {

////////////////////////////////////////////////////////////////////////////////

using TTableStat = std::pair<TString, TInstant>;

struct TGetTablesResponse
{
    const NProto::TError Error;
    const TVector<TTableStat> Tables;

    explicit TGetTablesResponse(const NProto::TError& error)
        : Error(error)
    {}

    explicit TGetTablesResponse(TVector<TTableStat> tables)
        : Tables(std::move(tables))
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct TDescribeTableResponse
{
    const NProto::TError Error;
    const TStatsTableScheme TableScheme;

    explicit TDescribeTableResponse(const NProto::TError& error)
        : Error(error)
    {}

    TDescribeTableResponse(
            TVector<NYdb::TColumn> columns,
            TVector<TString> keyColumns)
        : TableScheme(std::move(columns), std::move(keyColumns))
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct IYdbStorage
    : public IStartable
{
    virtual ~IYdbStorage() = default;

    virtual NThreading::TFuture<NProto::TError> CreateTable(
        const TString& table,
        const NYdb::NTable::TTableDescription& description) = 0;

    virtual NThreading::TFuture<NProto::TError> AlterTable(
        const TString& table,
        const NYdb::NTable::TAlterTableSettings& settings) = 0;

    virtual NThreading::TFuture<NProto::TError> DropTable(const TString& table) = 0;

    virtual NThreading::TFuture<NProto::TError> ExecuteUploadQuery(
        const TString& query,
        NYdb::TParams params) = 0;

    virtual NThreading::TFuture<TDescribeTableResponse> DescribeTable(const TString& table) = 0;

    virtual NThreading::TFuture<TGetTablesResponse> GetHistoryTables() = 0;
};

IYdbStoragePtr CreateYdbStorage(
    TYdbStatsConfigPtr config,
    ILoggingServicePtr logging);

}   // namespace NCloud::NBlockStore::NYdbStats
