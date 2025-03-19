#pragma once
#include <kernel/common_server/api/snapshots/storage/abstract/storage.h>
#include <kernel/common_server/api/snapshots/object.h>

namespace NCS {

    namespace NSnapshots {

        class TDBAsyncFetcherSelection: public ISelectionResult {
        private:
            CSA_MUTABLE_DEF(TDBAsyncFetcherSelection, NCS::NStorage::TObjectRecordsSet<TMappedObject>, Objects);
            CSA_DEFAULT(TDBAsyncFetcherSelection, NCS::NStorage::ITransaction::TPtr, Transaction);
            CSA_DEFAULT(TDBAsyncFetcherSelection, NCS::NStorage::IFutureQueryResult::TPtr, QueryResult);
        protected:
            virtual bool DoFetch(TVector<TMappedObject>& result) const override;
        public:
            TDBAsyncFetcherSelection(NCS::NStorage::ITransaction::TPtr transaction)
                : Transaction(transaction)
            {

            }
        };

        class TDBTableObjectsManager: public IObjectsManager {
        private:
            using TBase = IObjectsManager;
            static TFactory::TRegistrator<TDBTableObjectsManager> Registrator;
            CSA_DEFAULT(TDBTableObjectsManager, TString, DBName);
            CSA_DEFAULT(TDBTableObjectsManager, TString, TableName);
            NStorage::IDatabase::TPtr Database;
        protected:
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::Read(jsonInfo, "db_name", DBName, true, false)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "table_name", TableName, true, false)) {
                    return false;
                }
                return true;
            }
            virtual NJson::TJsonValue DoSerializeToJson() const override {
                NJson::TJsonValue result = NJson::JSON_MAP;
                TJsonProcessor::Write(result, "db_name", DBName);
                TJsonProcessor::Write(result, "table_name", TableName);
                return result;
            }

            virtual bool DoRemoveSnapshot(const ui32 snapshotId) const override;
            virtual bool DoCreateSnapshot(const ui32 snapshot) const override;

            virtual bool DoRemoveSnapshotObjects(const TVector<NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const override;
            virtual bool DoRemoveSnapshotObjectsBySRCondition(const ui32 snapshotId, const TSRCondition& customCondition) const override;

            virtual ISelectionResult::TPtr DoGetObjects(const TVector<NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const override;
            virtual bool DoGetAllObjects(const ui32 snapshotId, TVector<TMappedObject>& result, const TObjectsFilter& objectsFilter) const override;

            virtual bool DoUpsertObjects(const TIndex& snapshotIndex, const TVector<TMappedObject>& objects, const ui32 snapshotId, const TString& userId) const override;

            virtual bool DoPutObjects(const TVector<TMappedObject>& objects, const ui32 snapshotId, const TString& userId) const override;

            virtual bool DoInitialize(const IBaseServer& server) override {
                Database = server.GetDatabase(DBName);
                if (!Database) {
                    TFLEventLog::Error("incorrect db_name for snapshot objects manager")("db_name", DBName);
                    return false;
                }
                return true;
            }
        public:
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override;

            static TString GetTypeName() {
                return "db_snapshot_objects";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

        };
    }
}
