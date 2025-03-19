#include "structured.h"
#include <library/cpp/digest/md5/md5.h>

namespace NSQL {

    NSQL::TAccessLogger::TAccessLogger(const TString& dbName)
        : RequestsByType("dbstat." + dbName + ".requests", false)
        , ErrorsByType("dbstat." + dbName + ".errors", false)
        , ZeroRowsAffected("dbstat." + dbName + ".zero-rows", false)
        , RowsCountAffected("dbstat." + dbName + ".rows-count", false)
        , RequestsSuccess("dbstat." + dbName + ".success-full-count", false)
        , RequestsErrors("dbstat." + dbName + ".errors-full-count", false)
        , RequestRows("dbstat." + dbName + ".rows-full-count", false) {
    }

    void NSQL::TAccessLogger::LogRequest(EPSRequestType requestType, const NCS::NStorage::TTransactionQueryResult& queryResult) const {
        RequestsByType.Signal(requestType, 1);
        if (!queryResult->IsSucceed()) {
            ErrorsByType.Signal(requestType, 1);
        }

        if (queryResult->GetAffectedRows() == 0) {
            ZeroRowsAffected.Signal(requestType, 1);
        }
        RowsCountAffected.Signal(requestType, queryResult->GetAffectedRows());
    }

    bool TTableAccessor::BuildCondition(const TTableRecord& record, const ITransaction& transaction, TString& result) const {
        result = record.BuildCondition(transaction);
        return true;
    }

    bool TTableAccessor::GetValues(const TTableRecord& record, const ITransaction& transaction, TString& result, const TSet<TString>* fieldsAll) const {
        result = record.GetValues(transaction, fieldsAll);
        return true;
    }

    NCS::NStorage::TTransactionQueryResult TTableAccessor::BulkUpsertRows(const TRecordsSet& records, const IDatabase& db) {
        auto tr = db.CreateTransaction();
        auto result = AddRows(records, tr, "", nullptr);
        if(result->IsSucceed() && !tr->Commit()) {
            return MakeAtomicShared<TQueryResult>(false, 0u);
        }
        return result;
    }

    NCS::NStorage::TTransactionQueryResult NSQL::TTableAccessor::Upsert(const TTableRecord& record, ITransaction::TPtr transaction, const TTableRecord& unique, bool* isUpdate, IBaseRecordsSet* recordsSet) {
        TString condition;
        if (!BuildCondition(unique, *transaction, condition)) {
            return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
        }
        auto result = UpdateRow(condition, record.BuildSet(*transaction), transaction, recordsSet, nullptr);
        Logger.LogRequest(EPSRequestType::Upsert, result);

        if (!result->IsSucceed()) {
            return result;
        }
        if (isUpdate) {
            *isUpdate = result->GetAffectedRows();
        }
        if (result->GetAffectedRows() == 0) {
            return AddIfNotExists(record, transaction, unique, recordsSet);
        }
        return result;
    }

    NCS::NStorage::TTransactionQueryResult NSQL::TTableAccessor::AddIfNotExists(const TTableRecord& record, ITransaction::TPtr transaction, const TTableRecord& unique, IBaseRecordsSet* recordsSet) {
        CHECK_WITH_LOG(!unique.Empty());
        TString conditionInner;
        if (!BuildCondition(unique, *transaction, conditionInner)) {
            return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
        }
        TStringStream condition;
        condition << " NOT EXISTS (SELECT 1 FROM " << TableName << " WHERE " << conditionInner << ")";
        auto result = AddRow(record, transaction, condition.Str(), recordsSet);
        Logger.LogRequest(EPSRequestType::AddIfNotExists, result);
        return result;
    }

    NCS::NStorage::TTransactionQueryResult NSQL::TTableAccessor::AddIfNotExists(const TRecordsSet& records, ITransaction::TPtr transaction, const TTableRecord& unique, IBaseRecordsSet* recordsSet) {
        CHECK_WITH_LOG(!unique.Empty());
        TString conditionInner;
        if (!BuildCondition(unique, *transaction, conditionInner)) {
            return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
        }
        TStringStream condition;
        condition << " NOT EXISTS (SELECT 1 FROM " << TableName << " WHERE " << conditionInner << ")";
        auto result = AddRows(records, transaction, condition.Str(), recordsSet);
        Logger.LogRequest(EPSRequestType::AddIfNotExists, result);
        return result;
    }

    NCS::NStorage::TTransactionQueryResult NSQL::TTableAccessor::AddRow(const TTableRecord& record, ITransaction::TPtr transaction, const TString& condition, IBaseRecordsSet* recordsSet) {
        TStringStream ss;
        TString values;
        if (!GetValues(record, *transaction, values)) {
            return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
        }

        ss << "INSERT INTO " << TableName << " (" << record.GetKeys() << ")";
        if (!condition) {
            ss << " VALUES (" << values << ")";
        } else {
            ss << " SELECT " << values << " WHERE " << condition;
        }
        if (recordsSet) {
            ss << " RETURNING *";
        }

        auto result = transaction->Exec(ss.Str(), recordsSet);
        Logger.LogRequest(EPSRequestType::AddRow, result);
        return result;
    }

    NCS::NStorage::TTransactionQueryResult NSQL::TTableAccessor::AddRows(const TRecordsSet& records, ITransaction::TPtr transaction, const TString& condition, IBaseRecordsSet* recordsSet) {
        if (records.GetRecords().empty()) {
            return MakeAtomicShared<TQueryResult>(true, 0u);
        }
        TStringStream ss;
        const TSet<TString> fieldsAll = records.GetAllFieldNames();

        ss << "INSERT INTO " << TableName << " (" << JoinSeq(", ", fieldsAll) << ")";
        if (!condition) {
            ss << " VALUES ";
            for (ui32 i = 0; i < records.GetRecords().size(); ++i) {
                if (i) {
                    ss << ", ";
                }
                TString values;
                if (!GetValues(records.GetRecords()[i], *transaction, values, &fieldsAll)) {
                    return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
                }
                ss << "(" << values << ")";
            }
        } else {
            ss << " SELECT ";
            for (ui32 i = 0; i < records.GetRecords().size(); ++i) {
                if (i) {
                    ss << " UNION ALL SELECT ";
                }
                TString values;
                if (!GetValues(records.GetRecords()[i], *transaction, values, &fieldsAll)) {
                    return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
                }
                ss << values;
            }
            ss << " WHERE " << condition;
        }
        if (!!recordsSet) {
            ss << " RETURNING *";
        }

        auto result = transaction->Exec(ss.Str(), recordsSet);
        Logger.LogRequest(EPSRequestType::AddRow, result);
        return result;
    }

    NCS::NStorage::TTransactionQueryResult NSQL::TTableAccessor::DoUpdateRows(const TVector<TTableRecord>& conditions, const TVector<TTableRecord>& updates, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet, const TSet<TString>* returnFields) {
        TStringStream ss;
        ss << "UPDATE " << TableName << " as t SET ";
        {
            TVector<TString> updatesInfo;
            for (auto&& [key, _] : updates.front()) {
                updatesInfo.emplace_back(key + " = c." + key + "_update");
            }
            ss << JoinSeq(", ", updatesInfo);
        }
        ss << " FROM (VALUES ";
        {
            TVector<TString> inputData;
            for (ui32 i = 0; i < conditions.size(); ++i) {
                TString condition_values;
                if (!GetValues(conditions[i], *transaction, condition_values)) {
                    return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
                }
                TString update_values;
                if (!GetValues(updates[i], *transaction, update_values)) {
                    return MakeAtomicShared<NSQL::TQueryResult>(false, 0u);
                }

                inputData.emplace_back("(" + condition_values + "," + update_values + ")");
            }
            ss << JoinSeq(", ", inputData);
            TVector<TString> localFields;
            for (auto&& [k, _] : conditions.front()) {
                localFields.emplace_back(k + "_condition");
            }
            for (auto&& [k, _] : updates.front()) {
                localFields.emplace_back(k + "_update");
            }
            ss << ") AS c(" << JoinSeq(", ", localFields) << ")";
        }
        ss << "WHERE ";
        {
            TVector<TString> localConditions;
            for (auto&& [k, _] : conditions.front()) {
                localConditions.emplace_back("t." + k + " = c." + k + "_condition");
            }
            ss << JoinSeq(" AND ", localConditions);
        }
        if (recordsSet) {
            if (returnFields && conditions.size()) {
                ss << " RETURNING " + JoinSeq(",", *returnFields);
            } else {
                ss << " RETURNING *";
            }
        }
        auto result = transaction->Exec(ss.Str(), recordsSet);
        Logger.LogRequest(EPSRequestType::Update, result);
        return result;
    }

    NCS::NStorage::TTransactionQueryResult NSQL::TTableAccessor::UpdateRow(const TString& condition, const TString& update, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet, const TSet<TString>* returnFields) {
        TString query = "UPDATE " + TableName + " SET " + update + (!!condition ? (" WHERE " + condition) : "");
        if (recordsSet) {
            if (returnFields) {
                query += " RETURNING " + JoinSeq(", ", *returnFields);
            } else {
                query += " RETURNING *";
            }
        }
        auto result = transaction->Exec(query, recordsSet);
        Logger.LogRequest(EPSRequestType::Update, result);
        return result;
    }

    NSQL::ITableAccessor::TPtr NSQL::TDatabase::GetTable(const TString& tableName) const {
        return MakeAtomicShared<TTableAccessor>(tableName, Logger);
    }

    bool TDatabase::CreateTable(const TString& tableName, const TString& scriptConstruction, const TString& tableNameMacros) const {
        auto tr = TransactionMaker().NotReadOnly().SetLockTimeout(TDuration::Zero()).Build();
        TString realScript = scriptConstruction;
        SubstGlobal(realScript, "$" + tableNameMacros, tableName);
        auto qResult = tr->Exec(realScript);
        return qResult->IsSucceed() && tr->Commit();
    }

    bool TDatabase::CreateTable(const NCS::NStorage::TCreateTableQuery& query) const {
        TStringBuilder sb;
        sb << "CREATE TABLE IF NOT EXISTS $TABLE_NAME (" << Endl;
        bool isFirst = true;
        for (auto&& i : query.GetColumns()) {
            if (!isFirst) {
                sb << "," << Endl;
            }
            sb << i.GetId() << " ";
            switch (i.GetType()) {
                case NCS::NStorage::EColumnType::Text:
                    sb << " TEXT ";
                    break;
                case NCS::NStorage::EColumnType::Double:
                    sb << " DOUBLE PRECISION ";
                    break;
                case NCS::NStorage::EColumnType::Boolean:
                    sb << " BOOLEAN ";
                    break;
                case NCS::NStorage::EColumnType::I32:
                    if (i.IsAutogeneration()) {
                        sb << " SERIAL ";
                    } else {
                        sb << " INTEGER ";
                    }
                    break;
                case NCS::NStorage::EColumnType::I64:
                    if (i.IsAutogeneration()) {
                        sb << " BIGSERIAL ";
                    } else {
                        sb << " BIGINT ";
                    }
                    break;
                case NCS::NStorage::EColumnType::UI64:
                    sb << " NUMERIC(20,0) ";
                    break;
                case NCS::NStorage::EColumnType::Binary:
                    sb << " BYTEA ";
                    break;
            }
            if (!i.IsNullable()) {
                sb << " NOT NULL ";
            }
            if (i.IsPrimary()) {
                sb << " PRIMARY KEY ";
            } else if (i.IsUnique()) {
                sb << " UNIQUE ";
            }
            isFirst = false;
        }
        sb << Endl << ")" << Endl;
        return CreateTable(query.GetTableName(), sb, "TABLE_NAME");
    }

    bool TDatabase::CreateIndex(const NCS::NStorage::TCreateIndexQuery& query) const {
        TStringBuilder sb;
        sb << "CREATE ";
        if (query.IsUnique()) {
            sb << "UNIQUE ";
        }
        sb << "INDEX IF NOT EXISTS ";
        {
            TStringBuilder sbIndexName;
            sbIndexName << query.GetTableName();
            for (auto&& i : query.GetColumns()) {
                sbIndexName << "_" << i;
            }
            sb << "idx_" << MD5::Calc(sbIndexName) << " ON " << query.GetTableName() << "(";
        }
        bool isFirst = true;
        for (auto&& i : query.GetColumns()) {
            if (!isFirst) {
                sb << ", ";
            }
            sb << i;
            isFirst = false;
        }
        sb << ")";
        auto transaction = TransactionMaker().ReadOnly(false).SetLockTimeout(TDuration::Zero()).Build();
        return transaction->Exec(sb)->IsSucceed() && transaction->Commit();
    }

    bool TDatabase::AddColumn(const NCS::NStorage::TAddColumnQuery& query) const {
        TStringBuilder sb;
        sb << "ALTER TABLE " << query.GetTableName() << Endl;
        const auto& c = query.GetColumn();
        sb << "ADD COLUMN IF NOT EXISTS " << c.GetId() << Endl;
        switch (c.GetType()) {
            case NCS::NStorage::EColumnType::Text:
                sb << " TEXT ";
                break;
            case NCS::NStorage::EColumnType::Double:
                sb << " DOUBLE PRECISION ";
                break;
            case NCS::NStorage::EColumnType::Boolean:
                sb << " BOOLEAN ";
                break;
            case NCS::NStorage::EColumnType::I32:
                if (c.IsAutogeneration()) {
                    sb << " SERIAL ";
                } else {
                    sb << " INTEGER ";
                }
                break;
            case NCS::NStorage::EColumnType::I64:
                if (c.IsAutogeneration()) {
                    sb << " BIGSERIAL ";
                } else {
                    sb << " BIGINT ";
                }
                break;
            case NCS::NStorage::EColumnType::UI64:
                sb << " NUMERIC(20,0) ";
                break;
            case NCS::NStorage::EColumnType::Binary:
                sb << " BYTEA ";
                break;
        }
        if (!c.IsNullable()) {
            sb << " NOT NULL ";
        }
        if (c.IsPrimary()) {
            sb << " PRIMARY KEY ";
        } else if (c.IsUnique()) {
            sb << " UNIQUE ";
        }
        auto transaction = TransactionMaker().ReadOnly(false).SetLockTimeout(TDuration::Zero()).Build();
        return transaction->Exec(sb)->IsSucceed() && transaction->Commit();
    }

    bool TDatabase::DropTable(const TString& tableName) const {
        auto transaction = TransactionMaker().ReadOnly(false).SetLockTimeout(TDuration::Zero()).Build();
        auto queryResult = transaction->Exec("DROP TABLE IF EXISTS " + tableName);
        return queryResult->IsSucceed() && transaction->Commit();
    }

    TString TTransaction::BuildRequest(const NCS::NStorage::TRemoveRowsQuery& query) const {
        TStringBuilder sb;
        sb << "DELETE FROM " << query.GetTableName() << Endl;
        if (!!query.GetCondition()) {
            sb << "WHERE " << query.GetCondition() << Endl;
        }
        if (query.GetNeedReturn()) {
            sb << " RETURNING *";
        }
        return sb;
    }

}
