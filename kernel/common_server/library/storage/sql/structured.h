#pragma once

#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/library/unistat/signals.h>

namespace NSQL {
    using NStorage::IDatabase;
    using NStorage::IDatabaseConfig;
    using NStorage::IQueryResult;
    using NStorage::IBaseRecordsSet;
    using NStorage::ITableAccessor;
    using NStorage::ITransaction;
    using NStorage::TPackedRecordsSet;
    using NStorage::TTableRecord;

    enum class EPSRequestType {
        Update = 0 /* "update" */,
        Upsert = 1 /* "upsert" */,
        AddRow = 2 /* "add_row" */,
        Select = 3 /* "select" */,
        AddIfNotExists = 4 /* "add_if_not_exists" */,
        Delete = 5 /* "delete" */,
        BulkUpsert = 6 /* "bulk_upsert" */
    };

    class TAccessLogger {
    public:
        TAccessLogger(const TString& dbName);

        TAccessLogger(TAccessLogger&& logger) = default;
        TAccessLogger& operator=(TAccessLogger&& logger) = default;

        void SuccessSignal() const {
            RequestsSuccess.Signal(1);
        }

        void ErrorSignal() const {
            RequestsErrors.Signal(1);
        }

        void RowsCountSignal(const ui32 count) const {
            RequestRows.Signal(count);
        }

        void LogRequest(EPSRequestType requestType, const NCS::NStorage::TTransactionQueryResult& queryResult) const;

    private:
        TEnumSignal<EPSRequestType, double> RequestsByType;
        TEnumSignal<EPSRequestType, double> ErrorsByType;
        TEnumSignal<EPSRequestType, double> ZeroRowsAffected;
        TEnumSignal<EPSRequestType, double> RowsCountAffected;
        TUnistatSignal<double> RequestsSuccess;
        TUnistatSignal<double> RequestsErrors;
        TUnistatSignal<double> RequestRows;
    };

    class TQueryResult: public IQueryResult {
    public:
        TQueryResult() = default;
        TQueryResult(bool success, ui32 rows)
            : Succeed(success)
            , AffectedRows(rows) {
        }

        virtual ui32 GetAffectedRows() const override {
            return AffectedRows;
        }

        virtual bool IsSucceed() const override {
            return Succeed;
        }

    private:
        bool Succeed = false;
        ui32 AffectedRows = 0;
    };

    class TTransaction: public ITransaction {
    public:
        virtual TString BuildRequest(const NCS::NStorage::TRemoveRowsQuery& query) const override;
    };

    class TTableAccessor: public ITableAccessor {
    protected:
        virtual NCS::NStorage::TTransactionQueryResult DoUpdateRows(const TVector<TTableRecord>& conditions, const TVector<TTableRecord>& updates, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet, const TSet<TString>* returnFields) override;
        virtual bool BuildCondition(const TTableRecord& record, const ITransaction& transaction, TString& result) const;
        virtual bool GetValues(const TTableRecord& record, const ITransaction& transaction, TString& result, const TSet<TString>* fieldsAll = nullptr) const;
        const TString TableName;
        const TAccessLogger& Logger;
    public:
        TTableAccessor(const TString& tableName, const TAccessLogger& logger)
            : TableName(tableName)
            , Logger(logger) {
        }

        virtual const TString& GetTableName() const override {
            return TableName;
        }

        virtual NCS::NStorage::TTransactionQueryResult Upsert(const TTableRecord& record, ITransaction::TPtr transaction, const TTableRecord& unique, bool* isUpdate, IBaseRecordsSet* recordsSet) override;
        virtual NCS::NStorage::TTransactionQueryResult AddIfNotExists(const TTableRecord& record, ITransaction::TPtr transaction, const TTableRecord& unique, IBaseRecordsSet* recordsSet) override;
        virtual NCS::NStorage::TTransactionQueryResult AddIfNotExists(const TRecordsSet& records, ITransaction::TPtr transaction, const TTableRecord& unique, IBaseRecordsSet* recordsSet) override;
        virtual NCS::NStorage::TTransactionQueryResult AddRow(const TTableRecord& record, ITransaction::TPtr transaction, const TString& condition, IBaseRecordsSet* recordsSet) override;
        virtual NCS::NStorage::TTransactionQueryResult AddRows(const TRecordsSet& records, ITransaction::TPtr transaction, const TString& condition, IBaseRecordsSet* recordsSet) override;
        virtual NCS::NStorage::TTransactionQueryResult BulkUpsertRows(const TRecordsSet& records, const IDatabase& db) override;
        virtual NCS::NStorage::TTransactionQueryResult UpdateRow(const TString& condition, const TString& update, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet, const TSet<TString>* returnFields) override;
    };

    class TDatabase: public IDatabase {
    private:
        using TBase = IDatabase;
    public:
        TDatabase(const TString& name)
            : TBase(name)
            , Logger(name) {
        }

        virtual ITableAccessor::TPtr GetTable(const TString& tableName) const override;
        virtual bool DropTable(const TString& tableName) const override;
        virtual bool CreateTable(const TString& tableName, const TString& scriptConstruction, const TString& tableNameMacros) const override;
        virtual bool CreateTable(const NCS::NStorage::TCreateTableQuery& query) const override;
        virtual bool CreateIndex(const NCS::NStorage::TCreateIndexQuery& query) const override;
        virtual bool AddColumn(const NCS::NStorage::TAddColumnQuery& query) const override;
    protected:
        const TAccessLogger& GetLogger() const {
            return Logger;
        }
        TAccessLogger& GetLogger() {
            return Logger;
        }

    private:
        TAccessLogger Logger;
    };
}

