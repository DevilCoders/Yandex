#pragma once
#include "query_result.h"
#include "transaction.h"
#include <kernel/common_server/library/storage/records/record.h>
#include <kernel/common_server/library/storage/records/abstract.h>
#include <kernel/common_server/library/storage/records/set.h>

namespace NCS {
    namespace NStorage {

        class IDatabase;

        class ITableAccessor {
        private:
            virtual NCS::NStorage::TTransactionQueryResult UpdateRow(const TString& condition, const TString& update, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet = nullptr, const TSet<TString>* returnFields = nullptr) = 0;
        public:
            using TPtr = TAtomicSharedPtr<ITableAccessor>;
        protected:
            virtual NCS::NStorage::TTransactionQueryResult DoUpdateRows(const TVector<TTableRecord>& conditions, const TVector<TTableRecord>& updates, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet, const TSet<TString>* returnFields);
            bool ValidateRecordSets(const TVector<TTableRecord>& records) const;
        public:
            virtual ~ITableAccessor() {}

            virtual NCS::NStorage::TTransactionQueryResult UpdateRow(const TTableRecord& condition, const TTableRecord& update, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet = nullptr, const TSet<TString>* returnFields = nullptr) {
                return UpdateRow(condition.BuildCondition(*transaction), update.BuildSet(*transaction), transaction, recordsSet, returnFields);
            }

            virtual NCS::NStorage::TTransactionQueryResult UpsertRows(const TSet<TString>& uniqueFieldIds, const TVector<TTableRecord>& objects, ITransaction::TPtr transaction) final;
            virtual NCS::NStorage::TTransactionQueryResult UpdateRows(const TVector<TTableRecord>& conditions, const TVector<TTableRecord>& updates, ITransaction::TPtr transaction, IBaseRecordsSet* recordsSet = nullptr, const TSet<TString>* returnFields = nullptr) final;

            virtual const TString& GetTableName() const = 0;

            enum class EUpdateWithRevisionResult {
                Updated,
                Inserted,
                IncorrectRevision,
                Failed
            };

            EUpdateWithRevisionResult UpsertWithRevision(const TTableRecord& rowUpdateExt, const TSet<TString>& uniqueFieldsSet,
                const TMaybe<ui32> currentRevision, const TString& revFieldName, ITransaction::TPtr transaction,
                IBaseRecordsSet* recordsSet = nullptr, const bool force = false);

            virtual NCS::NStorage::TTransactionQueryResult Upsert(const TTableRecord& record, ITransaction::TPtr transaction, const TTableRecord& unique, bool* isUpdate = nullptr, IBaseRecordsSet* recordsSet = nullptr) = 0;
            virtual NCS::NStorage::TTransactionQueryResult AddIfNotExists(const TTableRecord& record, ITransaction::TPtr transaction, const TTableRecord& unique, IBaseRecordsSet* recordsSet = nullptr) = 0;
            virtual NCS::NStorage::TTransactionQueryResult AddIfNotExists(const TRecordsSet& records, ITransaction::TPtr transaction, const TTableRecord& unique, IBaseRecordsSet* recordsSet = nullptr) = 0;
            virtual NCS::NStorage::TTransactionQueryResult AddRow(const TTableRecord& record, ITransaction::TPtr transaction, const TString& condition = "", IBaseRecordsSet* recordsSet = nullptr) = 0;
            virtual NCS::NStorage::TTransactionQueryResult AddRows(const TRecordsSet& records, ITransaction::TPtr transaction, const TString& condition = "", IBaseRecordsSet* recordsSet = nullptr) = 0;

            virtual NCS::NStorage::TTransactionQueryResult BulkUpsertRows(const TRecordsSet& records, const IDatabase& db) = 0;
        };

    }
}
