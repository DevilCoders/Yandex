#pragma once
#include "table.h"
#include "transaction.h"
#include "lock.h"
#include <util/folder/path.h>
#include <kernel/common_server/library/storage/query/create_table.h>

namespace NCS {
    namespace NStorage {
        class IDatabase;

        class TTransactionFeatures {
        private:
            const IDatabase& Database;
            CSA_FLAG(TTransactionFeatures, ReadOnly, false);
            CSA_FLAG(TTransactionFeatures, ExpectFail, false);
            CSA_FLAG(TTransactionFeatures, RepeatableRead, false);
            CSA_FLAG(TTransactionFeatures, NonTransaction, false);
            CSA_MAYBE(TTransactionFeatures, TDuration, LockTimeout);
            CSA_MAYBE(TTransactionFeatures, TDuration, StatementTimeout);
            CS_ACCESS(TTransactionFeatures, TString, Namespaces, "public");
        public:
            TTransactionFeatures(const IDatabase& db)
                : Database(db) {

            }

            ITransaction::TPtr Build() const;
        };

        class IDatabase {
        public:
            using TPtr = TAtomicSharedPtr<IDatabase>;
            using TAvailableReplicasSet = ui32;
        private:
            CSA_READONLY_DEF(TString, Name);
            virtual ITransaction::TPtr DoCreateTransaction(const TTransactionFeatures& features) const = 0;
        public:
            IDatabase(const TString& dbName)
                : Name(dbName)
            {

            }

            virtual ~IDatabase() {}
            virtual TVector<TString> GetAllTableNames() const = 0;
            virtual ITableAccessor::TPtr GetTable(const TString& tableName) const = 0;
            virtual bool CreateTable(const TString& tableName, const TString& scriptConstruction, const TString& tableNameMacros = "TABLE_NAME") const = 0;
            virtual bool CreateTable(const TCreateTableQuery& query) const = 0;
            virtual bool CreateIndex(const TCreateIndexQuery& query) const = 0;
            virtual bool AddColumn(const TAddColumnQuery& query) const = 0;
            virtual bool DropTable(const TString& tableName) const = 0;
            TTransactionFeatures TransactionMaker() const;
            ITransaction::TPtr CreateTransaction(const bool readonly = false) const;
            ITransaction::TPtr CreateTransaction(const TTransactionFeatures& features) const;
            virtual TAbstractLock::TPtr Lock(const TString& lockName, const bool writeLock, const TDuration timeout, const TString& namespaces = "public") const = 0;
        };

    }
}
