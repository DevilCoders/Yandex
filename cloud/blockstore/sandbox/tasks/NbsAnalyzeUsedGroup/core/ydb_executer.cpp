#include "ydb_executer.h"

namespace NCloud::NBlockStore::NAnalyzeUsedGroup {

////////////////////////////////////////////////////////////////////////////////

TYDBExecuter::TYDBExecuter(
        const TString& endpoint,
        TString database,
        const TString& token)
    : Database(std::move(database))
    , YdbDriver{NYdb::TDriverConfig()
                    .SetEndpoint(endpoint)
                    .SetDatabase(Database)
                    .SetAuthToken(token)}
    , YdbClient{YdbDriver}
{}

void TYDBExecuter::Stop()
{
    YdbDriver.Stop(true);
}

TYDBExecuter::TRawData TYDBExecuter::GetData(
    const TString& table,
    TDuration timestampFrom,
    TDuration timestampTo)
{
    TStringStream query;
    query << "--!syntax_v1" << Endl;
    query << "SELECT *" << Endl;
    query << "FROM `" << table << "`" << Endl;
    query << "WHERE `Timestamp` >= " << timestampFrom.Seconds()
          << " AND `Timestamp` < " << timestampTo.Seconds() << Endl;
    query << "ORDER BY `Timestamp`;";

    auto stream = YdbClient.StreamExecuteScanQuery(
        query.Str()).GetValue(TDuration::Minutes(5));

    if (!stream.IsSuccess()) {
        Cerr << __FUNCTION__ << ": stream error " << stream.GetStatus()
            << " Query: " << query.Str() << Endl;
        return {};
    }

    TRawData result;
    while (true) {
        auto it = stream.ReadNext().GetValue(TDuration::Minutes(5));

        if (!it.HasResultSet()) {
            break;
        }

        NYdb::TResultSetParser parser(it.ExtractResultSet());
        while (parser.TryNextRow()) {
            TMaybe<TString> loadData =
                parser.ColumnParser("LoadData").GetOptionalJson();
            TMaybe<ui64> timestamp =
                parser.ColumnParser("Timestamp").GetOptionalUint64();
            TMaybe<TString> hostName =
                parser.ColumnParser("HostName").GetOptionalString();

            if (loadData &&
                timestamp &&
                hostName)
            {
                result[std::move(*hostName.Get())].push_back(TRawMetricsData{
                    std::move(*loadData.Get()),
                    *timestamp.Get()});
            }
        }
    }

    return result;
}

TYDBExecuter::TRawData TYDBExecuter::GetAllData(
    const TString& table,
    TDuration timestampFrom,
    TDuration timestampTo)
{
    TRawData result;
    TMaybe<NYdb::NTable::TTablePartIterator> tableIterator;
    const TString path = Database + "/" + table;

    auto operation = [&path, &tableIterator]
        (NYdb::NTable::TSession session)
    {
        auto result = session.ReadTable(
            path,
            NYdb::NTable::TReadTableSettings()
                .AppendColumns("LoadData")
                .AppendColumns("Timestamp")
                .AppendColumns("HostName")
        ).GetValueSync();

        if (result.IsSuccess())
        {
            tableIterator = result;
        }

        return result;
    };

    auto status = YdbClient.RetryOperationSync(operation);
    if (!status.IsSuccess())
    {
        Cerr << __FUNCTION__ << ": ReatTable error :"
            << status.GetIssues().ToString() << Endl;
        return result;
    }

    while (true) {
        auto it = tableIterator->ReadNext().GetValueSync();

        if (!it.IsSuccess())
        {
            break;
        }

        NYdb::TResultSetParser parser(it.ExtractPart());
        while (parser.TryNextRow()) {
            TMaybe<ui64> timestamp =
                parser.ColumnParser("Timestamp").GetOptionalUint64();

            if (timestamp > timestampTo.Seconds() ||
                timestamp < timestampFrom.Seconds())
            {
                    continue;
            }

            TMaybe<TString> loadData =
                parser.ColumnParser("LoadData").GetOptionalJson();
            TMaybe<TString> hostName =
                parser.ColumnParser("HostName").GetOptionalString();

            if (loadData &&
                timestamp &&
                hostName)
            {
                result[std::move(*hostName.Get())].push_back(TRawMetricsData{
                    std::move(*loadData.Get()),
                    *timestamp.Get()});
            }
        }
    }

    return result;
}


}   // NCloud::NBlockStore::NAnalyzeUsedGroup
