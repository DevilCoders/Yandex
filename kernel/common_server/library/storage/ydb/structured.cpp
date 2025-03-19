#include "structured.h"
#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <util/folder/path.h>
#include <util/string/escape.h>
#include <util/generic/cast.h>

namespace NCS {
namespace NStorage {

IDatabaseConfig::TFactory::TRegistrator<TYDBDatabaseConfig> TYDBDatabaseConfig::Registrator("YDB");

TMaybe<NYdb::NTable::TSession> TYDBDatabase::CreateSession(const TString& reason) const {
    auto result = Client.GetSession().GetValueSync();
    if (!result.IsSuccess()) {
        TFLEventLog::Error("Client.CreateSession result is not success")("session_for", reason)
            ("issues", result.GetIssues().ToString(true))("&code", "exception");
        return Nothing();
    }
    return result.GetSession();
}

TYDBDatabase::TYDBDatabase(const TString& dbInternalId, const TYDBDatabaseConfig& config, const NStorage::IDatabaseConstructionContext* context)
    : TBase(dbInternalId)
    , TablePath(config.GetTablePath())
    , Driver(config.BuildDriverConfig(context))
    , Client(Driver)
    , Logger(MakeHolder<NSQL::TAccessLogger>(config.GetDBName()))
    , SendPool(MakeHolder<TThreadPool>())
{
    SendPool->Start(config.GetThreads());
}


TYDBDatabase::~TYDBDatabase() {
    Driver.Stop(true);
}

const TString& TYDBDatabase::GetTablePath() const {
    return TablePath;
}

TVector<TString> TYDBDatabase::GetAllTableNames() const {
    NYdb::NScheme::TSchemeClient schemeClient(Driver);
    auto result = schemeClient.ListDirectory(TablePath).GetValueSync();
    if (!result.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::GetAllTableNames schemeClient.ListDirectory result is not success")
            ("issues", result.GetIssues().ToString(true))("&code", "exception");
        return {};
    }

    TVector<TString> names;
    for (const auto& entry : result.GetChildren()) {
        names.push_back(entry.Name);
    }
    return names;
}

NStorage::ITableAccessor::TPtr TYDBDatabase::GetTable(const TString& tableName) const {
    auto session = CreateSession("TYDBDatabase::GetTable");
    if (!session) {
        return nullptr;
    }
    const auto result = session->DescribeTable(TFsPath(TablePath) / tableName).GetValueSync();
    if (!result.IsSuccess()) {
        TFLEventLog::Error("error while describe YDB table")("table_name", tableName)
            ("issues", result.GetIssues().ToString(true))("&code", "exception");
        return nullptr;
    }
    return MakeAtomicShared<TYDBDatabase::TTableAccessor>(tableName, *Logger, result.GetTableDescription());
}

bool TYDBDatabase::CreateTable(const TString& tableName, const TString& constructionScript, const TString& /*tableNameMacros*/) const {
    NJson::TJsonValue json;
    if (!NJson::ReadJsonFastTree(constructionScript, &json)) {
        TFLEventLog::Error("invalid json in constructionScript")("script", constructionScript);
        return false;
    }
    auto session = CreateSession("TYDBDatabase::CreateTable");
    if (!session) {
        return false;
    }
    auto builder = session->GetTableBuilder();
    for (const auto& [name, column]: json["columns"].GetMap()) {
        NYdb::EPrimitiveType type;
        if (!TryFromString(column.GetString(), type)) {
            TFLEventLog::Error("invalid column type")("type", column.GetString());
            return false;
        }
        builder.AddNullableColumn(name, type);
    }
    TVector<TString> primary;
    for (const auto& column : json["primary_key"].GetArray()) {
        primary.emplace_back(column.GetString());
    }
    builder.SetPrimaryKeyColumns(primary);
    builder.SetUniformPartitions(5);
    builder.Build();
    const auto result = session->CreateTable(TFsPath(TablePath) / tableName, builder.Build()).GetValueSync();
    if (!result.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::CreateTable Session.CreateTable result is not success")("table_name", tableName)
            ("issues", result.GetIssues().ToString(true))("&code", "exception");
        return false;
    }
    return true;
}


bool TYDBDatabase::CreateTable(const NStorage::TCreateTableQuery& query) const {
    auto session = CreateSession("TYDBDatabase::CreateTable");
    if (!session) {
        return false;
    }
    auto builder = session->GetTableBuilder();
    TVector<TString> primaryIndex;
    for (const auto& i : query.GetColumns()) {
        NYdb::EPrimitiveType type;
        switch (i.GetType()) {
        case NStorage::EColumnType::Text:
            type = NYdb::EPrimitiveType::String;
            break;
        case NStorage::EColumnType::Double:
            type = NYdb::EPrimitiveType::Double;
            break;
        case NStorage::EColumnType::I32:
            type = NYdb::EPrimitiveType::Int32;
            break;
        case NStorage::EColumnType::I64:
            type = NYdb::EPrimitiveType::Int64;
            break;
        case NStorage::EColumnType::UI64:
            type = NYdb::EPrimitiveType::Uint64;
            break;
        case NStorage::EColumnType::Binary:
            type = NYdb::EPrimitiveType::String;
            break;
        case NStorage::EColumnType::Boolean:
            type = NYdb::EPrimitiveType::Bool;
            break;
        }
        builder.AddNullableColumn(i.GetId(), type);
        if (i.IsPrimary()) {
            primaryIndex.emplace_back(i.GetId());
        }
    }
    builder.SetPrimaryKeyColumns(primaryIndex);
//    builder.SetUniformPartitions(5);
    builder.Build();
    const auto result = session->CreateTable(TFsPath(TablePath) / query.GetTableName(), builder.Build()).GetValueSync();
    if (!result.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::CreateTable Session.CreateTable result is not success")("table_name", query.GetTableName())
            ("issues", result.GetIssues().ToString(true))("&code", "exception");
        return false;
    }
    return true;
}


bool TYDBDatabase::CreateIndex(const NStorage::TCreateIndexQuery& /*query*/) const {
    TFLEventLog::Error("TYDBDatabase::CreateIndex not implemented");
    return false;
}

NYdb::TType GetYDBType(const EColumnType type) {
    switch (type) {
        case EColumnType::Binary:
        case EColumnType::Text:
            return NYdb::TTypeBuilder().BeginOptional().Primitive(NYdb::EPrimitiveType::String).EndOptional().Build();
        case EColumnType::I32:
            return NYdb::TTypeBuilder().BeginOptional().Primitive(NYdb::EPrimitiveType::Int32).EndOptional().Build();
        case EColumnType::I64:
            return NYdb::TTypeBuilder().BeginOptional().Primitive(NYdb::EPrimitiveType::Int64).EndOptional().Build();
        case EColumnType::UI64:
            return NYdb::TTypeBuilder().BeginOptional().Primitive(NYdb::EPrimitiveType::Uint64).EndOptional().Build();
        case EColumnType::Double:
            return NYdb::TTypeBuilder().BeginOptional().Primitive(NYdb::EPrimitiveType::Double).EndOptional().Build();
        case EColumnType::Boolean:
            return NYdb::TTypeBuilder().BeginOptional().Primitive(NYdb::EPrimitiveType::Bool).EndOptional().Build();
    }
}

bool TYDBDatabase::AddColumn(const NCS::NStorage::TAddColumnQuery& query) const {
    auto session = CreateSession("TYDBDatabase::DropTable");
    if (!session) {
        return false;
    }
    NYdb::NTable::TAlterTableSettings atSettings;
    atSettings.AppendAddColumns(NYdb::NTable::TTableColumn(query.GetColumn().GetId(), GetYDBType(query.GetColumn().GetType())));
    auto result = session->AlterTable(TFsPath(TablePath) / query.GetTableName(), atSettings).GetValueSync();
    if (!result.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::AddColumn Session.AddColumn result is not success")("table_name", query.GetTableName())
            ("issues", result.GetIssues().ToString(true))("&code", "exception");
        return false;
    }
    return true;
}

bool TYDBDatabase::DropTable(const TString& tableName) const {
    auto session = CreateSession("TYDBDatabase::DropTable");
    if (!session) {
        return false;
    }
    auto result = session->DropTable(TFsPath(TablePath) / tableName).GetValueSync();
    if (!result.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::DropTable Session.DropTable result is not success")("table_name", tableName)
            ("issues", result.GetIssues().ToString(true))("&code", "exception");
        return false;
    }
    return true;
}

TYDBDatabaseConfig::TYDBDatabaseConfig()
    : AuthConfig(MakeHolder<TAuthConfigNone>())
{}

NSQL::IDatabase::TPtr TYDBDatabaseConfig::ConstructDatabase(const NStorage::IDatabaseConstructionContext* context) const {
    return MakeAtomicShared<TYDBDatabase>(GetDBInternalId(), *this, context);
}

void TYDBDatabaseConfig::DoInit(const TYandexConfig::Section* section) {
    CHECK_WITH_LOG(section);
    const auto& directives = section->GetDirectives();
    directives.GetValue("DBName", DBName);
    directives.GetValue("Endpoint", Endpoint);
    directives.GetValue("TablePath", TablePath);
    directives.GetValue("Threads", Threads);
    AuthConfig = TAuthConfig::Create(section);

    if (TablePath.empty()) {
        TablePath = DBName;
    }
}

void TYDBDatabaseConfig::DoToString(IOutputStream& os) const {
    os << "DBName: " << DBName << Endl;
    os << "Endpoint: " << Endpoint << Endl;
    os << "TablePath: " << TablePath << Endl;
    os << "Threads: " << Threads << Endl;
}


NYdb::TDriverConfig TYDBDatabaseConfig::BuildDriverConfig(const NStorage::IDatabaseConstructionContext* context) const {
    auto result = NYdb::TDriverConfig()
        .SetEndpoint(Endpoint)
        .SetDatabase(DBName);
    AuthConfig->PatchDriverConfig(result, context ? context->GetTvmManager(): nullptr);
    return result;
}

const TAuthConfig& TYDBDatabaseConfig::GetAuthConfig() const {
    return *AuthConfig;
}


TYDBDatabaseConfig& TYDBDatabaseConfig::SetAuthConfig(THolder<TAuthConfig>&& authConfig) {
    AssertCorrectConfig(!!authConfig, "try to set null auth config");
    AuthConfig = std::move(authConfig);
    return *this;
}

NStorage::ITransaction::TPtr TYDBDatabase::DoCreateTransaction(const TTransactionFeatures& features) const {
    auto session = CreateSession("TYDBDatabase::CreateTransaction");
    if (!session) {
        return MakeAtomicShared<NStorage::TFailedTransaction>(*this);
    }
    if (features.IsNonTransaction()) {
        TFLEventLog::Error("non-transaction mode is not supported");
        return MakeAtomicShared<NStorage::TFailedTransaction>(*this);
    }
    auto beginResult = session->BeginTransaction(NYdb::NTable::TTxSettings::SerializableRW()).GetValueSync();
    if (!beginResult.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::CreateTransaction Session.BeginTransaction result is not success")
            ("issues", beginResult.GetIssues().ToString(true))("&code", "exception");
        return MakeAtomicShared<NStorage::TFailedTransaction>(*this);
    }

    return MakeAtomicShared<TYDBDatabase::TTransaction>(*this, *session, beginResult.GetTransaction());
}

NRTProc::TAbstractLock::TPtr TYDBDatabase::Lock(const TString& /*lockName*/, const bool /*writeLock*/, const TDuration /*timeout*/, const TString& /*namespaces*/) const {
    return nullptr;
}

bool TYDBDatabase::TTransaction::DoCommit() {
    NYdb::TStatus result = Transaction.Commit().GetValueSync();
    if (!result.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::TTransaction::DoCommit Transaction.Commit result is not success")
            ("issues", result.GetIssues().ToString(true))("&code", "exception");
    }
    return result.IsSuccess();
}

bool TYDBDatabase::TTransaction::DoRollback() {
    NYdb::TStatus result = Transaction.Rollback().GetValueSync();
    if (!result.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::TTransaction::DoRollback Transaction.Rollback result is not success")
            ("issues", result.GetIssues().ToString(true))("&code", "exception");
    }
    return result.IsSuccess();
}

void TYDBValueConverter::ParsePrimitiveValueToString(IOutputStream& ss) const {
    switch (Value.GetPrimitiveType()) {
        case NYdb::EPrimitiveType::Bool:
            ss << ::ToString(Value.GetBool());
            break;
        case NYdb::EPrimitiveType::Int8:
            ss << ::ToString(Value.GetInt8());
            break;
        case NYdb::EPrimitiveType::Uint8:
            ss << ::ToString(Value.GetUint8());
            break;
        case NYdb::EPrimitiveType::Int16:
            ss << ::ToString(Value.GetInt16());
            break;
        case NYdb::EPrimitiveType::Uint16:
            ss << ::ToString(Value.GetUint16());
            break;
        case NYdb::EPrimitiveType::Int32:
            ss << ::ToString(Value.GetInt32());
            break;
        case NYdb::EPrimitiveType::Uint32:
            ss << ::ToString(Value.GetUint32());
            break;
        case NYdb::EPrimitiveType::Int64:
            ss << ::ToString(Value.GetInt64());
            break;
        case NYdb::EPrimitiveType::Uint64:
            ss << ::ToString(Value.GetUint64());
            break;
        case NYdb::EPrimitiveType::Float:
            ss << ::ToString(Value.GetFloat());
            break;
        case NYdb::EPrimitiveType::Double:
            ss << ::ToString(Value.GetDouble());
            break;
        case NYdb::EPrimitiveType::Date:
            ss << ::ToString(Value.GetDate());
            break;
        case NYdb::EPrimitiveType::Datetime:
            ss << ::ToString(Value.GetDatetime());
            break;
        case NYdb::EPrimitiveType::Timestamp:
            ss << ::ToString(Value.GetTimestamp());
            break;
        case NYdb::EPrimitiveType::Interval:
            ss << ::ToString(Value.GetInterval());
            break;
        case NYdb::EPrimitiveType::TzDate:
            ss << Value.GetTzDate();
            break;
        case NYdb::EPrimitiveType::TzDatetime:
            ss << Value.GetTzDatetime();
            break;
        case NYdb::EPrimitiveType::TzTimestamp:
            ss << Value.GetTzTimestamp();
            break;
        case NYdb::EPrimitiveType::String:
            ss << Value.GetString();
            break;
        case NYdb::EPrimitiveType::Utf8:
            ss << Value.GetUtf8();
            break;
        case NYdb::EPrimitiveType::Yson:
            ss << Value.GetYson();
            break;
        case NYdb::EPrimitiveType::Json:
            ss << Value.GetJson();
            break;
        case NYdb::EPrimitiveType::Uuid:
            // no GetUuid()
            ss << "unsupported Uuid";
            break;
        case NYdb::EPrimitiveType::JsonDocument:
            ss << Value.GetJsonDocument();
            break;
        case NYdb::EPrimitiveType::DyNumber:
            ss << Value.GetDyNumber();
            break;
    }
}

void TYDBValueConverter::ParseValueToString(IOutputStream& ss) const {
    bool isFirst = true;
    switch (Value.GetKind()) {
        case NYdb::TTypeParser::ETypeKind::Primitive:
            ParsePrimitiveValueToString(ss);
            break;

        case NYdb::TTypeParser::ETypeKind::Decimal:
            ss << Value.GetDecimal().ToString();
            break;
        case NYdb::TTypeParser::ETypeKind::Pg:
            ss << Value.GetPg().Content_;
            break;
        case NYdb::TTypeParser::ETypeKind::Optional:
            Value.OpenOptional();
            if (Value.IsNull()) {
                ss << "";
            } else {
                ParseValueToString(ss);
            }
            Value.CloseOptional();
            break;

        case NYdb::TTypeParser::ETypeKind::List:
            Value.OpenList();
            ss << "[";
            while (Value.TryNextListItem()) {
                if (!isFirst) {
                    ss << ", ";
                    isFirst = false;
                }
                ParseValueToString(ss);
            }
            ss << "]";
            Value.CloseList();
            break;

        case NYdb::TTypeParser::ETypeKind::Struct:
            Value.OpenStruct();
            ss << "[";
            while (Value.TryNextMember()) {
                if (!isFirst) {
                    ss << ", ";
                    isFirst = false;
                }
                ss << Value.GetMemberName() << " = ";
                ParseValueToString(ss);
            }
            ss << "]";
            Value.CloseStruct();
            break;

        case NYdb::TTypeParser::ETypeKind::Tuple:
            Value.OpenTuple();
            ss << "[";
            while (Value.TryNextElement()) {
                if (!isFirst) {
                    ss << ", ";
                    isFirst = false;
                }
                ParseValueToString(ss);
            }
            ss << "]";
            Value.CloseTuple();
            break;

        case NYdb::TTypeParser::ETypeKind::Dict:
            Value.OpenDict();
            ss << "[";
            while (Value.TryNextDictItem()) {
                if (!isFirst) {
                    ss << ", ";
                    isFirst = false;
                }
                ss << "{";
                ParseValueToString(ss);
                Value.DictKey();
                ParseValueToString(ss);
                Value.DictPayload();
                ParseValueToString(ss);
                ss << "}";
            }
            ss << "]";
            Value.CloseDict();
            break;

        case NYdb::TTypeParser::ETypeKind::Variant:
            ss << "unsupported Variant";
            break;

        case NYdb::TTypeParser::ETypeKind::Void:
            ss << "void";
            break;

        case NYdb::TTypeParser::ETypeKind::Null:
            ss << "null";
            break;

        case NYdb::TTypeParser::ETypeKind::EmptyList:
            ss << "empty list";
            break;

        case NYdb::TTypeParser::ETypeKind::EmptyDict:
            ss << "empty dict";
            break;

        case NYdb::TTypeParser::ETypeKind::Tagged:
            ss << "unsupported tagged";
            break;
    }
}

void TYDBDatabase::TTransaction::ParseResult(const NYdb::TResultSet& resultSet, NStorage::IRecordsSetWT* records) const {
    if (!records) {
        return;
    }
    records->Reserve(resultSet.RowsCount());
    const TVector<NYdb::TColumn>& columns = resultSet.GetColumnsMeta();
    NYdb::TResultSetParser parser(resultSet);
    while (parser.TryNextRow()) {
        NStorage::TTableRecordWT record;
        for (size_t ind = 0; ind < parser.ColumnsCount(); ++ind) {
            NYdb::TValueParser& value = parser.ColumnParser(columns[ind].Name);
            TString valueString = TYDBValueConverter(value).ToString();
            record.Set(columns[ind].Name, valueString);
        }
        records->AddRow(std::move(record));
    }
}

class TYDBQueryResult: public NStorage::IOriginalContainer {
public:
    TYDBQueryResult(TAtomicSharedPtr<TVector<TVector<TString>>>&& data)
        : Data(data) {
    }

    const TAtomicSharedPtr<TVector<TVector<TString>>>& GetData() const {
        return Data;
    }

private:
    TAtomicSharedPtr<TVector<TVector<TString>>> Data;
};

void TYDBDatabase::TTransaction::ParseResult(const NYdb::TResultSet& resultSet, NStorage::IPackedRecordsSet* records) const {
    if (!records) {
        return;
    }

    TMap<TString, ui32> columnsDecoder;
    const TVector<NYdb::TColumn>& columns = resultSet.GetColumnsMeta();
    TVector<NCS::NStorage::TOrderedColumn> orderedColumns;
    for (size_t ind = 0; ind < columns.size(); ++ind) {
        NYdb::TTypeParser tParser(columns[ind].Type);
        NCS::NStorage::EColumnType cType = NCS::NStorage::EColumnType::Text;
        if (tParser.GetKind() == NYdb::TTypeParser::ETypeKind::Optional) {
            tParser.OpenOptional();
        }
        switch (tParser.GetPrimitive()) {
            case NYdb::EPrimitiveType::Bool:
                cType = NCS::NStorage::EColumnType::Boolean;
                break;
            case NYdb::EPrimitiveType::Int8:
            case NYdb::EPrimitiveType::Uint8:
            case NYdb::EPrimitiveType::Int16:
            case NYdb::EPrimitiveType::Uint16:
            case NYdb::EPrimitiveType::Int32:
            case NYdb::EPrimitiveType::Uint32:
            case NYdb::EPrimitiveType::Timestamp:
                cType = NCS::NStorage::EColumnType::I32;
                break;
            case NYdb::EPrimitiveType::Int64:
            case NYdb::EPrimitiveType::Uint64:
                cType = NCS::NStorage::EColumnType::I64;
                break;
            case NYdb::EPrimitiveType::Float:
            case NYdb::EPrimitiveType::Double:
                cType = NCS::NStorage::EColumnType::Double;
                break;
            case NYdb::EPrimitiveType::Datetime:
            case NYdb::EPrimitiveType::String:
            case NYdb::EPrimitiveType::Uuid:
            case NYdb::EPrimitiveType::Json:
            default:
                cType = NCS::NStorage::EColumnType::Text;
                break;
        }
        orderedColumns.emplace_back(columns[ind].Name, cType);
    }
    records->Initialize(resultSet.RowsCount(), orderedColumns);

    NYdb::TResultSetParser parser(resultSet);
    auto data = MakeAtomicShared<TVector<TVector<TString>>>();
    data->resize(parser.RowsCount());
    TVector<TStringBuf> record;
    record.resize(columns.size());
    size_t rowIndex = 0;
    while (parser.TryNextRow()) {
        data->at(rowIndex).resize(columns.size());
        for (size_t ind = 0; ind < parser.ColumnsCount(); ++ind) {
            NYdb::TValueParser& value = parser.ColumnParser(columns[ind].Name);
            data->at(rowIndex)[ind] = TYDBValueConverter(value).ToString();
            record[ind] = TStringBuf(data->at(rowIndex)[ind]);
        }
        rowIndex++;
        records->AddRow(record);
    }

    auto container = MakeHolder<TYDBQueryResult>(std::move(data));
    records->StoreOriginalData(std::move(container));
}

void TYDBDatabase::TTransaction::ParseResult(const NYdb::TResultSet& resultSet, const NStorage::ITransaction::TStatement& statement) const {
    if (!statement.GetRecords()) {
        return;
    } else if (NStorage::IRecordsSetWT* records = statement.GetRecords()->GetAs<NStorage::IRecordsSetWT>()) {
        ParseResult(resultSet, records);
    } else if (NStorage::IPackedRecordsSet* records = statement.GetRecords()->GetAs<NStorage::IPackedRecordsSet>()) {
        ParseResult(resultSet, records);
    } else {
        S_FAIL_LOG << "Incorrect records storage class (not implemented in code)" << Endl;
    }
}

NStorage::IQueryResult::TPtr TYDBDatabase::TTransaction::DoExec(const NStorage::ITransaction::TStatement& statement) {
    if (statement.GetQuery().find("CREATE TABLE") == TString::npos && statement.GetQuery().find("ALTER TABLE") == TString::npos) {
        return ProcessExecResult(Session.ExecuteDataQuery(statement.GetQuery(), NYdb::NTable::TTxControl::Tx(Transaction)).GetValueSync(), statement);
    } else {
        return ProcessExecResult(Session.ExecuteSchemeQuery(statement.GetQuery()).GetValueSync(), statement);
    }
}

NStorage::IQueryResult::TPtr TYDBDatabase::TTransaction::ProcessExecResult(const NYdb::NTable::TDataQueryResult& qResult, const NStorage::ITransaction::TStatement& statement) {
    if (!qResult.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::TTransaction::DoExec Session.ExecuteDataQuery result is not success")
            ("issues", qResult.GetIssues().ToString(true))("query", statement.GetQuery())("&code", "exception");
        return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
    }

    const TVector<NYdb::TResultSet>& resultSets = qResult.GetResultSets();
    size_t affected = 0;
    if (resultSets.size() > 0) {
        affected = resultSets[0].RowsCount();
        ParseResult(resultSets[0], statement);
    }
    if (resultSets.size() > 1) {
        TFLEventLog::Error("TYDBDatabase::TTransaction::DoExec Session.ExecuteDataQuery return more than one resultSet")
            ("query", statement.GetQuery())("&code", "exception");
    }
    if (Database.Logger) {
        Database.Logger->RowsCountSignal(affected);
        Database.Logger->SuccessSignal();
    }
    return MakeAtomicShared<NSQL::TQueryResult>(true, (ui32)affected);
}

NStorage::IQueryResult::TPtr TYDBDatabase::TTransaction::ProcessExecResult(const NYdb::TStatus& qResult, const NStorage::ITransaction::TStatement& statement) {
    if (!qResult.IsSuccess()) {
        TFLEventLog::Error("TYDBDatabase::TTransaction::DoExec Session.ExecuteDataQuery result is not success")
            ("issues", qResult.GetIssues().ToString(true))("query", statement.GetQuery())("&code", "exception");
        return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
    }

    if (Database.Logger) {
        Database.Logger->SuccessSignal();
    }
    return MakeAtomicShared<NSQL::TQueryResult>(true, 0u);
}

NStorage::ITransaction::TStatement TYDBDatabase::TTransaction::PrepareStatement(const NStorage::ITransaction::TStatement& statementExt) const {
    TStringStream ss;
    ss << "PRAGMA TablePathPrefix(\"" << Database.GetTablePath() << "\");\n\n";
    ss << statementExt.GetQuery();
    ss << "\n";
    return TStatement(ss.Str(), statementExt.GetRecords());
}

const NStorage::IDatabase& TYDBDatabase::TTransaction::GetDatabase() {
    return Database;
}

class TYDBValueQuoter {
public:
    template <class T>
    TString operator()(const T& value) const {
        return ::ToString(value);
    }
    TString operator()(const TString& value) const {
        return "\"" + EscapeC(value) + "\"";
    }
    TString operator()(const TGUID& value) const {
        return "\"" + value.AsUuidString() + "\"";
    }
    TString operator()(const TBinaryData& value) const {
        if (value.IsNullData()) {
            return "NULL";
        } else {
            return "String::Base64Decode(\"" + Base64Encode(value.GetOriginalData()) + "\")";
        }
    }
    TString operator()(const TNull& /*value*/) const {
        return "NULL";
    }
};

TString TYDBDatabase::TTransaction::QuoteImpl(const TDBValueInput& v) const {
    TYDBValueQuoter pred;
    return std::visit(pred, v);
}

TYDBDatabase::TTransaction::TTransaction(const TYDBDatabase& database, const NYdb::NTable::TSession& session, const NYdb::NTable::TTransaction& transaction)
    : Database(database)
    , Session(session)
    , Transaction(transaction)
{}

TYDBDatabase::TTransaction::~TTransaction() {
    if (GetStatus() == NStorage::ETransactionStatus::InProgress) {
        Rollback();
    }
}

TTransactionQueryResult TYDBDatabase::TTransaction::ExecRequest(const NRequest::INode& request) noexcept {
    {
        const TSRDelete* srDelete = dynamic_cast<const TSRDelete*>(&request);
        if (srDelete) {
            TSRSelect srSelect(srDelete->GetTableName().GetTableName(), srDelete->GetRecordsSet());
            srSelect.SetCondition(srDelete->GetCondition());
            auto resultSelect = TBase::ExecRequest(srSelect);
            if (!resultSelect->IsSucceed()) {
                return resultSelect;
            }
            srDelete->SetRecordsSet(nullptr);

            auto resultDelete = TBase::ExecRequest(*srDelete);
            if (!resultDelete->IsSucceed()) {
                return resultDelete;
            }
            srDelete->SetRecordsSet(srSelect.GetRecordsSet());
            return resultSelect;
        }
    }
    {
        const TSRUpdate* srUpdate = dynamic_cast<const TSRUpdate*>(&request);
        if (srUpdate) {
            TSRSelect srSelect(srUpdate->GetTableName().GetTableName(), srUpdate->GetRecordsSet());
            srSelect.SetCondition(srUpdate->GetCondition());
            auto resultSelect = TBase::ExecRequest(srSelect);
            if (!resultSelect->IsSucceed()) {
                return resultSelect;
            }
            srUpdate->SetRecordsSet(nullptr);

            auto resultDelete = TBase::ExecRequest(*srUpdate);
            if (!resultDelete->IsSucceed()) {
                return resultDelete;
            }
            srUpdate->SetRecordsSet(srSelect.GetRecordsSet());
            return resultSelect;
        }
    }
    return TBase::ExecRequest(request);
}

namespace {
    bool TypeToString(const TStringBuf key, const NCS::NStorage::TDBValueInput& value, const TStringBuf tableName, NYdb::TTypeParser& type, const NStorage::ITransaction& transaction, IOutputStream& ss) {
        if (type.GetKind() == NYdb::TTypeParser::ETypeKind::Optional) {
            type.OpenOptional();
            bool result = TypeToString(key, value, tableName, type, transaction, ss);
            type.CloseOptional();
            return result;
        }
        if (type.GetKind() != NYdb::TTypeParser::ETypeKind::Primitive) {
            TFLEventLog::Error("column kind not supported")("key", key)("table_name", tableName)("kind", type.GetKind());
            return false;
        }
        const TString valueImpl = transaction.Quote(value);
        switch (type.GetPrimitive()) {
            case NYdb::EPrimitiveType::String:
                ss << valueImpl;
                break;
            default:
                ss << "CAST(" << valueImpl << " AS " << type.GetPrimitive() << ")";
        }
        return true;
    }
}

bool TYDBDatabase::TTableAccessor::GetValues(const NStorage::TTableRecord& record, const NStorage::ITransaction& transaction, TString& result, const TSet<TString>* fieldsAll) const {
    TStringStream ss;
    for (const auto& [key, value] : record) {
        if (fieldsAll && !fieldsAll->contains(key)) {
            continue;
        }
        if (!ss.Empty()) {
            ss << ", ";
        }
        const auto* column = MapFindPtr(Columns, key);
        if (!column) {
            TFLEventLog::Error("record key not found it table schema")("key", key)("table_name", TableName);
            return false;
        }
        NYdb::TTypeParser type(column->Type);
        if (!TypeToString(key, value, TableName, type, transaction, ss)) {
            return false;
        }
    }
    result = ss.Str();
    return true;
}
#define VALUE_TO_BUILDER_CASE(Type, ValueType) \
    case NYdb::EPrimitiveType::Type: {\
        ValueType typedValue;\
        if (!TryFromString(TDBValueInputOperator::SerializeToString(value), typedValue)) {\
            TFLEventLog::Error("cannot convert value")("key", key)("value", TDBValueInputOperator::SerializeToString(value))("table_name", TableName)("type", type.GetPrimitive());\
            return false;\
        }\
        result.Type(typedValue);\
        break;\
    }

#define VALUE_TO_BUILDER_STRING_CASE(Type) \
    case NYdb::EPrimitiveType::Type:\
        result.Type(TDBValueInputOperator::SerializeToString(value));\
        break;

#define VALUE_TO_BUILDER_INSTANT_CASE(Type) \
    case NYdb::EPrimitiveType::Type: {\
        TInstant typedValue;\
        if (!TInstant::TryParseHttp(TDBValueInputOperator::SerializeToString(value), typedValue)) {\
            TFLEventLog::Error("cannot convert value")("key", key)("value", TDBValueInputOperator::SerializeToString(value))("table_name", TableName)("type", type.GetPrimitive());\
            return false;\
        }\
        result.Type(typedValue);\
        break;\
    }

template<class TBuilder>
bool TYDBDatabase::TTableAccessor::ToBuilder(TBuilder& result, const TStringBuf key, const NCS::NStorage::TDBValueInput& valueExt, NYdb::TTypeParser& type) const {
    const TString value = NCS::NStorage::TDBValueInputOperator::SerializeToString(valueExt);
    if (type.GetKind() == NYdb::TTypeParser::ETypeKind::Optional) {
        type.OpenOptional();
        result.BeginOptional();
        if (!ToBuilder(result, key, valueExt, type)) {
            return false;
        }
        result.EndOptional();
        type.CloseOptional();
        return true;
    }
    if (type.GetKind() != NYdb::TTypeParser::ETypeKind::Primitive) {
        TFLEventLog::Error("column kind not supported")("key", key)("table_name", TableName)("kind", type.GetKind());
        return false;
    }
    switch (type.GetPrimitive()) {
    VALUE_TO_BUILDER_CASE(Bool, bool);
    VALUE_TO_BUILDER_CASE(Int8, i8);
    VALUE_TO_BUILDER_CASE(Uint8, ui8);
    VALUE_TO_BUILDER_CASE(Int16, i16);
    VALUE_TO_BUILDER_CASE(Uint16, ui16);
    VALUE_TO_BUILDER_CASE(Int32, i32);
    VALUE_TO_BUILDER_CASE(Uint32, ui32);
    VALUE_TO_BUILDER_CASE(Int64, i64);
    VALUE_TO_BUILDER_CASE(Uint64, ui64);
    VALUE_TO_BUILDER_CASE(Float, float);
    VALUE_TO_BUILDER_CASE(Double, double);
    VALUE_TO_BUILDER_INSTANT_CASE(Date);
    VALUE_TO_BUILDER_INSTANT_CASE(Datetime);
    VALUE_TO_BUILDER_INSTANT_CASE(Timestamp);
    case NYdb::EPrimitiveType::Interval: {
        TDuration typedValue;
        if (!TryFromString(value, typedValue)) {
            TFLEventLog::Error("cannot convert value")("key", key)("value", value)("table_name", TableName)("type", type.GetPrimitive());
            return false;
        }
        result.Interval(typedValue.MilliSeconds());

    }
    VALUE_TO_BUILDER_STRING_CASE(TzDate);
    VALUE_TO_BUILDER_STRING_CASE(TzDatetime);
    VALUE_TO_BUILDER_STRING_CASE(TzTimestamp);
    case NYdb::EPrimitiveType::Uuid:
        result.String(TString(value));
        break;
    case NYdb::EPrimitiveType::String: {
        const TString data(NCS::NStorage::TDBValueOperator::SerializeToString(value));
        result.String(data);
        break;
    }
    VALUE_TO_BUILDER_STRING_CASE(Utf8);
    VALUE_TO_BUILDER_STRING_CASE(Yson);
    VALUE_TO_BUILDER_STRING_CASE(Json);
    VALUE_TO_BUILDER_STRING_CASE(JsonDocument);
    VALUE_TO_BUILDER_STRING_CASE(DyNumber);
    }
    return true;
}

#undef VALUE_TO_BUILDER_CASE
#undef VALUE_TO_BUILDER_STRING_CASE
#undef VALUE_TO_BUILDER_INSTANT_CASE

template<class TBuilder>
bool TYDBDatabase::TTableAccessor::ToBuilder(TBuilder& result, const NStorage::TTableRecord& record) const {
    auto& structBuilder = result.BeginStruct();
    for (const auto& [key, field] : record) {
        const auto* column = MapFindPtr(Columns, key);
        if (!column) {
            TFLEventLog::Error("record key not found it table schema")("key", key)("table_name", TableName);
            return false;
        }
        NYdb::TTypeParser type(column->Type);
        ToBuilder(structBuilder.AddMember(key), key, field, type);
    }
    result.EndStruct();
    return true;
}

template<class TBuilder>
bool TYDBDatabase::TTableAccessor::ToBuilder(TBuilder& result, const TRecordsSet& records, const ui32 from, const ui32 to) const {
    result.BeginList();
    const auto begin = records.begin() + from;
    const auto end = records.begin() + Min<ui32>(to, records.size());
    for (auto record = begin; record != end; ++record) {
        if (!ToBuilder(result.AddListItem(), *record)) {
            return false;
        }
    }
    result.EndList();
    return true;
}

bool TYDBDatabase::TTableAccessor::BuildCondition(const NStorage::TTableRecord& record, const NStorage::ITransaction& transaction, TString& result) const {
    TStringStream ss;
    for (const auto& [key, value] : record) {
        if (!ss.Empty()) {
            ss << " AND ";
        }
        const auto* column = MapFindPtr(Columns, key);
        if (!column) {
            TFLEventLog::Error("record key not found it table schema")("key", key)("table_name", TableName);
            return false;
        }
        ss << key << " == ";
        NYdb::TTypeParser type(column->Type);
        if (!TypeToString(key, value, TableName, type, transaction, ss)) {
            return false;
        }
    }
    result = ss.Str();
    return true;
}

bool TYDBDatabase::TTableAccessor::RecordToAsStruct(const NStorage::TTableRecord& record, NStorage::ITransaction::TPtr transaction, TString& result) const {
    TStringStream ss;
    for (const auto& [key, value] : record) {
        if (!ss.Empty()) {
            ss << ", ";
        }
        const auto* column = MapFindPtr(Columns, key);
        if (!column) {
            TFLEventLog::Error("record key not found it table schema")("key", key)("table_name", TableName);
            return false;
        }
        NYdb::TTypeParser type(column->Type);
        if (!TypeToString(key, value, TableName, type, *transaction, ss)) {
            return false;
        }
        ss << " AS " << key;
    }
    result = "AsStruct(" + ss.Str() + ")";
    return true;
}

TTransactionQueryResult TYDBDatabase::TTableAccessor::Upsert(const NStorage::TTableRecord& record, NStorage::ITransaction::TPtr transaction, const NStorage::TTableRecord& unique, bool* isUpdate, NStorage::IBaseRecordsSet* recordsSet) {
    TString condition;
    if(!BuildCondition(unique, *transaction, condition)) {
        return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
    }

    TStringBuilder checkQuery;
    checkQuery << "SELECT * FROM " << TableName << " WHERE " << condition;
    auto checkResult = transaction->Exec(checkQuery, nullptr);

    TStringBuilder selectQuery;
    selectQuery << "SELECT " << record.GetKeys() << " FROM AS_TABLE($data)";
    if (condition) {
        selectQuery << " WHERE " << condition;
    }

    TStringBuilder query;
    TString structStr;
    if (!RecordToAsStruct(record, transaction, structStr)) {
        return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
    }
    query << "$data = AsList(" + structStr + ");";
    if (recordsSet) {
        query << Endl << selectQuery << ";";
    }
    query << Endl << "UPSERT INTO " << TableName << " (" << record.GetKeys() << ") ";
    query << selectQuery << ";";

    if (isUpdate) {
        *isUpdate = checkResult->GetAffectedRows() > 0;
    }

    auto result = transaction->Exec(query, recordsSet);
    Logger.LogRequest(NSQL::EPSRequestType::Upsert, result);
    return result;
}

TTransactionQueryResult TYDBDatabase::TTableAccessor::AddRow(const NStorage::TTableRecord& record, NStorage::ITransaction::TPtr transaction, const TString& condition, NStorage::IBaseRecordsSet* recordsSet) {
    TStringBuilder selectQuery;
    selectQuery << "SELECT " << record.GetKeys() << " FROM AS_TABLE($data)";
    if (condition) {
        selectQuery << " WHERE " << condition;
    }

    TStringBuilder query;
    TString structStr;
    if (!RecordToAsStruct(record, transaction, structStr)) {
        return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
    }
    query << "$data = AsList(" << structStr << ");";
    if (recordsSet) {
        query << Endl << selectQuery << ";";
    }
    query << Endl << "INSERT INTO " << TableName << " (" << record.GetKeys() << ") ";
    query << selectQuery << ";";

    auto result = transaction->Exec(query, recordsSet);
    Logger.LogRequest(NSQL::EPSRequestType::AddRow, result);
    return result;
}

TTransactionQueryResult TYDBDatabase::TTableAccessor::BulkUpsertRows(const TRecordsSet& records, const IDatabase& db) {
    if (records.GetRecords().empty()) {
        return MakeAtomicShared<NSQL::TQueryResult>(true, 0u);
    }

    constexpr ui32 batchSize = 1000;
    const ui32 batchCount = 1 + (records.size() - 1) / batchSize; //Деление с округлением вверх
    TVector<NYdb::NTable::TAsyncBulkUpsertResult> results(batchCount);
    TVector<NThreading::TPromise<void>> sent(batchCount);
    TVector<NThreading::TFuture<void>> sentFuture(batchCount);
    const auto* ydb = VerifyDynamicCast<const TYDBDatabase*>(&db);
    for (ui32 b = 0; b < batchCount; ++b) {
        sent[b] = NThreading::NewPromise<void>();
        sentFuture[b] = sent[b].GetFuture();
        auto resultPtr = &results[b];
        auto sentPtr = &sent[b];
        ydb->SendPool->SafeAddFunc([b, &records, this, resultPtr, sentPtr, ydb] () {
            NYdb::TValueBuilder builder;
            if (ToBuilder(builder, records, b * batchSize, (b + 1) * batchSize)) {
                *resultPtr = ydb->Client.BulkUpsert(TFsPath(ydb->GetTablePath()) / TableName, builder.Build());
            } else {
                auto p = NThreading::NewPromise<NYdb::NTable::TBulkUpsertResult>();
                p.SetValue(NYdb::NTable::TBulkUpsertResult(NYdb::TStatus(NYdb::EStatus::BAD_REQUEST, NYql::TIssues())));
                *resultPtr = p.GetFuture();
            }
            sentPtr->SetValue();
        });
    }
    (void)NThreading::WaitAll(sentFuture).Wait();
    (void)NThreading::WaitAll(results).Wait();
    bool ok = true;
    for (const auto& r: results) {
        const auto& qResult = r.GetValue();
        if (!qResult.IsSuccess()) {
            ok = false;
            TFLEventLog::Error("TYDBDatabase::TTableAccessor::BulkUpsertRows Session.BulkUpsert result is not success")
                ("issues", qResult.GetIssues().ToString(true))("&code", "exception");
        }
    }
    if (!ok) {
        return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
    }

    auto result = MakeAtomicShared<NSQL::TQueryResult>(true, (ui32)records.size());
    Logger.LogRequest(NSQL::EPSRequestType::BulkUpsert, result);
    return result;
}

TTransactionQueryResult TYDBDatabase::TTableAccessor::AddRows(const TRecordsSet& records, NStorage::ITransaction::TPtr transaction, const TString& condition, NStorage::IBaseRecordsSet* recordsSet) {
    if (records.GetRecords().empty()) {
        return MakeAtomicShared<NSQL::TQueryResult>(true, 0u);
    }

    TVector<TString> recordsAsStruct;
    for (const auto& record : records.GetRecords()) {
        TString structStr;
        if (!RecordToAsStruct(record, transaction, structStr)) {
            return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
        }
        recordsAsStruct.push_back(structStr);
    }

    const TSet<TString> fieldsAll = records.GetAllFieldNames();

    TStringBuilder selectQuery;
    selectQuery << "SELECT " << JoinSeq(", ", fieldsAll) << " FROM AS_TABLE($data)";
    if (condition) {
        selectQuery << " WHERE " << condition;
    }

    TStringBuilder query;
    query << "$data = AsList(" << JoinSeq(",", recordsAsStruct) << ");";
    //if (recordsSet) {
        query << Endl << selectQuery << ";";
    //}
    query << Endl << "INSERT INTO " << TableName << " (" + JoinSeq(", ", fieldsAll) << ") " << selectQuery << ";";

    auto result = transaction->Exec(query, recordsSet);
    Logger.LogRequest(NSQL::EPSRequestType::AddRow, result);
    return result;
}

TTransactionQueryResult TYDBDatabase::TTableAccessor::UpdateRow(const TString& condition, const TString& update, NStorage::ITransaction::TPtr transaction, NStorage::IBaseRecordsSet* recordsSet, const TSet<TString>* returnFields) {
    // YDB cannot modify table twicely per transaction
    // YDB does not allow to SELECT from modified table
    // So SELECT performed before modification and has non modified data.

    TStringBuilder query;
    query << Endl << "SELECT " << (returnFields ? JoinSeq(", ", *returnFields) : "*") << " FROM " << TableName;
    if (condition) {
        query << " WHERE " << condition;
    }
    query << ";" << Endl << "UPDATE " << TableName << " SET " << update;
    if (condition) {
        query << " WHERE " << condition;
    }
    query << ";";

    auto result = transaction->Exec(query, recordsSet);
    Logger.LogRequest(NSQL::EPSRequestType::Update, result);
    return result;
}

TYDBDatabase::TTableAccessor::TTableAccessor(const TString& tableName, const NSQL::TAccessLogger& logger, const NYdb::NTable::TTableDescription& description)
    : NSQL::TTableAccessor(tableName, logger)
{
    for (const auto& c: description.GetTableColumns()) {
        Columns.emplace(c.Name, c);
    }
}

}
}
