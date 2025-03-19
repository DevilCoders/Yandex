#include "ydb_writer.h"
#include <util/folder/pathsplit.h>
#include <util/string/printf.h>


const char* YdbTypeName(NYdb::EPrimitiveType t) {
#define RETURN__TYPE_NAME(a) case NYdb::EPrimitiveType::a : return #a;
    switch (t) {
    RETURN__TYPE_NAME(Bool);
    RETURN__TYPE_NAME(Int8);
    RETURN__TYPE_NAME(Uint8);
    RETURN__TYPE_NAME(Int16);
    RETURN__TYPE_NAME(Uint16);
    RETURN__TYPE_NAME(Int32);
    RETURN__TYPE_NAME(Uint32);
    RETURN__TYPE_NAME(Int64);
    RETURN__TYPE_NAME(Uint64);
    RETURN__TYPE_NAME(Float);
    RETURN__TYPE_NAME(Double);
    RETURN__TYPE_NAME(Date);
    RETURN__TYPE_NAME(Datetime);
    RETURN__TYPE_NAME(Timestamp);
    RETURN__TYPE_NAME(Interval);
    RETURN__TYPE_NAME(TzDate);
    RETURN__TYPE_NAME(TzDatetime);
    RETURN__TYPE_NAME(TzTimestamp);
    RETURN__TYPE_NAME(String);
    RETURN__TYPE_NAME(Utf8);
    RETURN__TYPE_NAME(Yson);
    RETURN__TYPE_NAME(Json);
    RETURN__TYPE_NAME(Uuid);
    default:
        return "INVALID_TYPE";
    }
#undef RETURN_NAME
}

TString BuildStreamsQuery(TString tablePath) {
    return Sprintf(R"(
        PRAGMA TablePathPrefix("%s");

        DECLARE $data AS "List<Struct<
            _p_logGroupId: String?,
            _p_streamName: String?,
            _p_updatedAt: String?>>";

        REPLACE INTO log_streams (logGroupId, streamName, updatedAt)
        SELECT
            _p_logGroupId,
            _p_streamName,
            _p_updatedAt
        FROM AS_TABLE($data);

    )", tablePath.c_str());
}

TString BuildWriteQuery(const TYdbWriter::TSchema& schema) {
    TStringStream str;
    str << "DECLARE $items AS 'List<Struct<\n";
    for (auto cit = schema.Columns.begin(); cit != schema.Columns.end(); ++cit) {
        str << "   " << cit->second.ParamName << " : Optional<" << YdbTypeName(cit->second.Type) << ">";
        if (std::next(cit) != schema.Columns.end()) {
            str << ",\n";
        }
    }
    str << "\n>>';\n"
        << "REPLACE INTO [__TABLE_NAME__]\n"
        << "SELECT\n";

    for (auto cit = schema.Columns.begin(); cit != schema.Columns.end(); ++cit) {
        str << "   " << cit->second.ParamName << " AS [" << cit->first << "]";
        if (std::next(cit) != schema.Columns.end()) {
            str << ",\n";
        }
    }

    str << "\n"
        << "FROM AS_TABLE($items);\n";

    return str.Str();
}

TString BuildCreateTableQuery(const TYdbWriter::TSchema& schema) {
    TStringStream str;
    str << "CREATE TABLE [__TABLE_NAME__] (\n";
    for (auto cit = schema.Columns.begin(); cit != schema.Columns.end(); ++cit) {
        str << "    " << cit->first << " " << YdbTypeName(cit->second.Type);
        str << ",\n";
    }
    str << "    PRIMARY KEY (\n";
    for (size_t i = 0; i < schema.KeyColumns.size(); ++i) {
        str << "        " << schema.KeyColumns[i] << (i + 1 != schema.KeyColumns.size() ? "," : "") << "\n";
    }
    str << "    )\n);\n";
    return str.Str();
}

TYdbWriter::TYdbWriter(const NYdb::TDriver &driver, TString tablePath, TString tableProfile, ui64 sessionLimit, TSchema&& schema)
    : Schema(std::move(schema))
    , TablePath(tablePath)
    , TableProfileName(tableProfile)
    , QueryTemplate(BuildWriteQuery(Schema))
    , StreamsQuery(BuildStreamsQuery(tablePath))
    , Client(driver, NYdb::NTable::TClientSettings().SessionPoolSettings( NYdb::NTable::TSessionPoolSettings().MaxActiveSessions(sessionLimit)))
{
}


static NYdb::TParams PackValuesToParamsAsList(const TVector<NYdb::TValue>& items, const TString name = "$items") {
    NYdb::TValueBuilder itemsAsList;
    itemsAsList.BeginList();
    for (const NYdb::TValue& item: items) {
        itemsAsList.AddListItem(item);
    }
    itemsAsList.EndList();

    NYdb::TParamsBuilder paramsBuilder;
    paramsBuilder.AddParam(name, itemsAsList.Build());
    return paramsBuilder.Build();
}

TString TYdbWriter::TouchTable(TString name) {
    {
        TGuard<TMutex> g(KnownTablesMutex);
        auto ti = KnownTables.FindPtr(name);
        if (ti) {
            ti->LastAccess = TInstant::Now();
            return ti->Query;
        }
    }

    DoCreateTable(name);

    // this time it should succeed
    return TouchTable(name);
}

static TString JoinPath(const TString& basePath, const TString& path) {
    if (basePath.empty()) {
        return path;
    }

    TPathSplitUnix prefixPathSplit(basePath);
    prefixPathSplit.AppendComponent(path);

    return prefixPathSplit.Reconstruct();
}

void TYdbWriter::TouchStreamsTable() {
    TAtomicBase created = AtomicGet(StreamsTableCreated);
    if (created) {
        return;
    }
    DoCreateStreamsTable();
    AtomicSet(StreamsTableCreated, 1);
}

void TYdbWriter::DoCreateStreamsTable() {

    NYdb::NTable::TRetryOperationSettings retrySettings;
    retrySettings.MaxRetries(10);

    auto fullPath = JoinPath(TablePath, "log_streams");

    auto status = Client.RetryOperationSync([fullPath](NYdb::NTable::TSession session) {
        auto seriesDesc = NYdb::NTable::TTableBuilder()
                .AddNullableColumn("logGroupId", NYdb::EPrimitiveType::String)
                .AddNullableColumn("streamName", NYdb::EPrimitiveType::String)
                .AddNullableColumn("updatedAt", NYdb::EPrimitiveType::String)
                .SetPrimaryKeyColumns(TVector<TString>{"logGroupId", "streamName"})
                .Build();

        return session.CreateTable(fullPath, std::move(seriesDesc)).GetValueSync();
    }, retrySettings);

    if (!status.IsSuccess()) {
        Y_FAIL("Failed to create table [%s]: %s", fullPath.c_str(), status.GetIssues().ToString().c_str());
    }
}


void TYdbWriter::DoCreateTable(TString name) {
    auto createTable = [this, name] (NYdb::NTable::TSession session) noexcept {
        auto tableDesc = NYdb::NTable::TTableBuilder();
        for (auto cit = Schema.Columns.begin(); cit != Schema.Columns.end(); ++cit) {
            tableDesc.AddNullableColumn(cit->first, cit->second.Type);
        }
        tableDesc.SetPrimaryKeyColumns(Schema.KeyColumns);

        auto tableSettings = NYdb::NTable::TCreateTableSettings();
        if (!TableProfileName.empty()) {
            tableSettings.PresetName(TableProfileName);
        }
        tableSettings.PartitioningPolicy(
            NYdb::NTable::TPartitioningPolicy().AutoPartitioning(NYdb::NTable::EAutoPartitioningPolicy::AutoSplit));

        return session.CreateTable(name,
            tableDesc.Build(), std::move(tableSettings)).GetValueSync();
    };


    NYdb::NTable::TRetryOperationSettings retrySettings;
    retrySettings.MaxRetries(10);

    auto status = Client.RetryOperationSync(createTable, retrySettings);

    if (!status.IsSuccess()) {
        Y_FAIL("Failed to create table [%s]: %s", name.c_str(), status.GetIssues().ToString().c_str());
    }

    TString query = QueryTemplate;
    SubstGlobal(query, "__TABLE_NAME__", name);

    TString createTableQuery = BuildCreateTableQuery(Schema);
    SubstGlobal(createTableQuery, "__TABLE_NAME__", name);
    Cerr << createTableQuery << Endl;
    Cerr << "QUERY: " << query << Endl;

    {
        TGuard<TMutex> g(KnownTablesMutex);
        auto& newTable = KnownTables[name];
        newTable.Query = query;
        newTable.LastAccess = TInstant::Now();
    }
}


NYdb::TAsyncStatus TYdbWriter::Write(TString table, const TVector<NYdb::TValue>& batch) {
    TString name = TablePath + "/" + table;
    TString query = TouchTable(name);

    Cerr << "WRITE: " << name  << " " << batch.size() << " rows" << Endl;
    Cerr << "Cient session count: " << Client.GetActiveSessionCount() << " queries in flight: " << QueriesInFlight << Endl;

    auto& inFlight = QueriesInFlight;

    return Client.GetSession().Apply([name, query, &batch, &inFlight](auto sessionFuture) noexcept -> NThreading::TFuture<NYdb::TStatus> {
        auto seesionResult = sessionFuture.ExtractValue();

        if (!seesionResult.IsSuccess()) {
            return NThreading::MakeFuture<NYdb::TStatus>(seesionResult);
        }

        auto session = seesionResult.GetSession();

        auto params = PackValuesToParamsAsList(batch);

        AtomicIncrement(inFlight);

        return session.ExecuteDataQuery(
                    query,
                    NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
                    std::move(params),
                    NYdb::NTable::TExecDataQuerySettings().KeepInQueryCache(true).ClientTimeout(TDuration::Seconds(15))
        ).Apply([name, &batch, &inFlight](NYdb::NTable::TAsyncDataQueryResult f) noexcept -> NThreading::TFuture<NYdb::TStatus> {
            AtomicDecrement(inFlight);

            auto val = f.ExtractValue(); //
            TString issues = val.GetIssues().ToString();
            if (!issues.empty()) {
                Cerr << "WRITE " << batch.size() << " rows to [" << name << "]:\n" << issues << Endl;
            }
            return NThreading::MakeFuture<NYdb::TStatus>(val);
        });

    });
}

NYdb::TAsyncStatus TYdbWriter::WriteStreams(const TVector<NYdb::TValue> streams) {
    TouchStreamsTable();

    auto query = StreamsQuery;

    auto& inFlight = QueriesInFlight;

    return Client.GetSession().Apply([query, streams, &inFlight](auto sessionFuture) noexcept -> NThreading::TFuture<NYdb::TStatus> {
        auto sessionResult = sessionFuture.ExtractValue();

        if (!sessionResult.IsSuccess()) {
            return NThreading::MakeFuture<NYdb::TStatus>(sessionResult);
        }

        auto session = sessionResult.GetSession();

        auto params = PackValuesToParamsAsList(streams, "$data");

        AtomicIncrement(inFlight);

        return session.ExecuteDataQuery(
                query,
                NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
                std::move(params),
                NYdb::NTable::TExecDataQuerySettings().KeepInQueryCache(true).ClientTimeout(TDuration::Seconds(15))
        ).Apply([streams, &inFlight](NYdb::NTable::TAsyncDataQueryResult f) noexcept -> NThreading::TFuture<NYdb::TStatus> {
            AtomicDecrement(inFlight);

            auto val = f.ExtractValue(); //
            TString issues = val.GetIssues().ToString();
            if (!issues.empty()) {
                Cerr << "WRITE " << streams.size() << " rows to [log_streams]:\n" << issues << Endl;
            }
            return NThreading::MakeFuture<NYdb::TStatus>(val);
        });

    });
}
