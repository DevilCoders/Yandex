#pragma once

#include "util/generic/ptr.h"
#include <kernel/common_server/library/storage/sql/structured.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/library/storage/records/t_record.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/kikimr_auth/config.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

namespace NCS {
    namespace NStorage {
        class TYDBDatabaseConfig: public IDatabaseConfig {
        public:
            TYDBDatabaseConfig();
            virtual NSQL::IDatabase::TPtr ConstructDatabase(const IDatabaseConstructionContext* context) const override;
            NYdb::TDriverConfig BuildDriverConfig(const IDatabaseConstructionContext* context) const;
            const TAuthConfig& GetAuthConfig() const;
            TYDBDatabaseConfig& SetAuthConfig(THolder<TAuthConfig>&& authConfig);
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) override;
            virtual void DoToString(IOutputStream& os) const override;
        private:
            CSA_DEFAULT(TYDBDatabaseConfig, TString, DBName);
            CSA_DEFAULT(TYDBDatabaseConfig, TString, Endpoint);
            CSA_DEFAULT(TYDBDatabaseConfig, TString, TablePath);
            CS_ACCESS(TYDBDatabaseConfig, ui32, Threads, 5);

            THolder<TAuthConfig> AuthConfig;
            static TFactory::TRegistrator<TYDBDatabaseConfig> Registrator;
        };

        class TYDBDatabase: public IDatabase {
        private:
            using TBase = IDatabase;
            TString TablePath;
            NYdb::TDriver Driver;
            mutable NYdb::NTable::TTableClient Client;
            mutable THolder<NSQL::TAccessLogger> Logger;

            class TTransaction;
            class TTableAccessor;
            TMaybe<NYdb::NTable::TSession> CreateSession(const TString& reason) const;
            THolder<TThreadPool> SendPool;
        protected:
            virtual ITransaction::TPtr DoCreateTransaction(const TTransactionFeatures& features) const override;
        public:
            TYDBDatabase(const TString& dbInternalId, const TYDBDatabaseConfig& config, const IDatabaseConstructionContext* context);
            ~TYDBDatabase();
            const TString& GetTablePath() const;

            virtual TVector<TString> GetAllTableNames() const override;
            virtual ITableAccessor::TPtr GetTable(const TString& tableName) const override;
            virtual bool CreateTable(const TString& tableName, const TString& constructionScript, const TString& tableNameMacros) const override;
            virtual bool CreateTable(const TCreateTableQuery& query) const override;
            virtual bool CreateIndex(const TCreateIndexQuery& query) const override;
            virtual bool AddColumn(const NCS::NStorage::TAddColumnQuery& query) const override;
            virtual bool DropTable(const TString& tableName) const override;
            virtual NRTProc::TAbstractLock::TPtr Lock(const TString& lockName, const bool writeLock, const TDuration timeout, const TString& namespaces = "public") const override;
        };

        class TYDBValueConverter {
        private:
            NYdb::TValueParser& Value;

            void ParsePrimitiveValueToString(IOutputStream& ss) const;
            void ParseValueToString(IOutputStream& ss) const;

        public:
            TYDBValueConverter(NYdb::TValueParser& value)
                : Value(value) {
            }

            TString ToString() {
                TStringStream ss;
                ParseValueToString(ss);
                return ss.Str();
            }
        };

        class TYDBDatabase::TTransaction: public NSQL::TTransaction {
        private:
            using TBase = NSQL::TTransaction;
            const TYDBDatabase& Database;
            NYdb::NTable::TSession Session;
            NYdb::NTable::TTransaction Transaction;

            void ParseResult(const NYdb::TResultSet& resultSet, IRecordsSetWT* records) const;
            void ParseResult(const NYdb::TResultSet& resultSet, IPackedRecordsSet* records) const;
            void ParseResult(const NYdb::TResultSet& resultSet, const ITransaction::TStatement& statement) const;
            NStorage::IQueryResult::TPtr ProcessExecResult(const NYdb::NTable::TDataQueryResult& qResult, const ITransaction::TStatement& statement);
            NStorage::IQueryResult::TPtr ProcessExecResult(const NYdb::TStatus& qResult, const ITransaction::TStatement& statement);

        protected:
            virtual bool DoCommit() override;
            virtual bool DoRollback() override;
            virtual NSQL::IQueryResult::TPtr DoExec(const TStatement& statement) override;

        public:
            TTransaction(const TYDBDatabase& database, const NYdb::NTable::TSession& session, const NYdb::NTable::TTransaction& transaction);
            virtual ~TTransaction();

            virtual TTransactionQueryResult ExecRequest(const NRequest::INode& request) noexcept override;
            virtual TStatement PrepareStatement(const TStatement& statementExt) const override;
            virtual const IDatabase& GetDatabase() override;
            virtual TString QuoteImpl(const TDBValueInput& v) const override;
        };

        class TYDBDatabase::TTableAccessor: public NSQL::TTableAccessor {
        private:
            bool RecordToAsStruct(const TTableRecord& record, ITransaction::TPtr transaction, TString& result) const;

            template<class TBuilder>
            bool ToBuilder(TBuilder& result, const TStringBuf key, const NCS::NStorage::TDBValueInput& value, NYdb::TTypeParser& type) const;

            template<class TBuilder>
            bool ToBuilder(TBuilder& result, const TTableRecord& record) const;

            template<class TBuilder>
            bool ToBuilder(TBuilder& result, const TRecordsSet& records, const ui32 from = 0, const ui32 to = Max<ui32>()) const;

            TMap<TString, NYdb::NTable::TTableColumn> Columns;

        protected:
            virtual bool BuildCondition(const TTableRecord& record, const ITransaction& transaction, TString& result) const override;
            virtual bool GetValues(const TTableRecord& record, const ITransaction& transaction, TString& result, const TSet<TString>* fieldsAll) const override;

        public:
            TTableAccessor(const TString& tableName, const NSQL::TAccessLogger& logger, const NYdb::NTable::TTableDescription& description);

            virtual TTransactionQueryResult Upsert(const TTableRecord& record, ITransaction::TPtr transaction, const TTableRecord& unique, bool* isUpdate, IBaseRecordsSet* recordsSet) override;
            virtual TTransactionQueryResult AddRow(const TTableRecord& record, ITransaction::TPtr transaction, const TString& condition, IBaseRecordsSet* recordsSet) override;
            virtual TTransactionQueryResult AddRows(const TRecordsSet& records, ITransaction::TPtr transaction, const TString& condition, IBaseRecordsSet* recordsSet) override;
            virtual TTransactionQueryResult UpdateRow(const TString& condition, const TString& update, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet, const TSet<TString>* returnFields) override;
            virtual TTransactionQueryResult BulkUpsertRows(const TRecordsSet& records, const IDatabase& db) override;
        };
    }
}
