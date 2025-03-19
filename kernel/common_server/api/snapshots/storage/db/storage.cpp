#include "storage.h"

#include <kernel/common_server/library/storage/query/request.h>

namespace NCS {
    namespace NSnapshots {
        TDBTableObjectsManager::TFactory::TRegistrator<TDBTableObjectsManager> TDBTableObjectsManager::Registrator(TDBTableObjectsManager::GetTypeName());

        bool TDBTableObjectsManager::DoRemoveSnapshotObjects(const TVector<NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const {
            if (objectIds.empty()) {
                return true;
            }
            NStorage::ITransaction::TPtr tr = Database->CreateTransaction(false);
            TSRDelete srDelete(TableName + ::ToString(snapshotId));
            srDelete.RetCondition<TSRMulti>().FillRecords(objectIds);
            if (!tr->ExecRequest(srDelete)->IsSucceed()) {
                TFLEventLog::Error("cannot execute transaction");
                return false;
            }
            if (!tr->Commit()) {
                TFLEventLog::Error("cannot commit snapshot removing")("reason", tr->GetStringReport());
                return false;
            }
            return true;
        }

        bool TDBTableObjectsManager::DoRemoveSnapshotObjectsBySRCondition(const ui32 snapshotId, const TSRCondition& customCondition) const {
            NStorage::ITransaction::TPtr tr = Database->CreateTransaction(false);
            const auto table = TableName + ::ToString(snapshotId);
            TSRDelete reqDelete(table);
            reqDelete.SetCondition(customCondition);
            if (!tr->ExecRequest(reqDelete)) {
                TFLEventLog::Error("cannot execute snapshot removing request");
                return false;
            }
            if (!tr->Commit()) {
                TFLEventLog::Error("cannot commit snapshot removing request");
                return false;
            }
            return true;
        }

        bool TDBTableObjectsManager::DoGetAllObjects(const ui32 snapshotId, TVector<TMappedObject>& result, const TObjectsFilter& objectsFilter) const {
            NStorage::ITransaction::TPtr tr = Database->CreateTransaction(true);
            NStorage::TObjectRecordsSet<TMappedObject> objects;
            TSRSelect selectQuery(TableName + ::ToString(snapshotId), &objects);
            if (objectsFilter.HasLimit()) {
                selectQuery.SetCountLimit(objectsFilter.GetLimitUnsafe());
            }
            if (objectsFilter.HasShift()) {
                selectQuery.SetOffset(objectsFilter.GetShiftUnsafe());
            }
            auto qResult = tr->ExecRequest(selectQuery);
            if (!qResult->IsSucceed()) {
                TFLEventLog::Error("cannot execute transaction");
                return false;
            }
            result = objects.DetachObjects();
            return true;
        }

        ISelectionResult::TPtr TDBTableObjectsManager::DoGetObjects(const TVector<NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const {
            if (objectIds.empty()) {
                return MakeAtomicShared<TEmptySelectionResult>();
            }
            const bool useNamespace = Singleton<TDatabaseNamespace>()->HasCustomNamespace(Database->GetName());
            NStorage::ITransaction::TPtr tr = Database->TransactionMaker().ReadOnly().NonTransaction(!useNamespace).Build();
            auto fetcher = MakeHolder<TDBAsyncFetcherSelection>(tr);
            TSRSelect query(TableName + ::ToString(snapshotId), &fetcher->MutableObjects());
            auto& srMulti = query.RetCondition<TSRMulti>(ESRMulti::Or);
            for (auto&& i : objectIds) {
                srMulti.RetNode<TSRMulti>(ESRMulti::And).FillBinary(i);
            }
            fetcher->SetQueryResult(tr->AsyncExecRequest(query));
            return fetcher.Release();
        }

        bool TDBTableObjectsManager::DoUpsertObjects(const TIndex& snapshotIndex, const TVector<TMappedObject>& objects, const ui32 snapshotId, const TString& /*userId*/) const {
            NStorage::ITransaction::TPtr tr = Database->CreateTransaction(false);
            TSRInsert reqInsert(TableName + ::ToString(snapshotId));
            reqInsert.SetConflictPolicy(TSRInsert::EConflictPolicy::Update).FillRecords(objects);
            reqInsert.MutableUniqueFieldIds().InitFromSet(snapshotIndex.GetFieldsSet());
            auto result = tr->ExecRequest(reqInsert);
            if (!result->IsSucceed()) {
                TFLEventLog::Error("cannot insert records into snapshot")("reason", tr->GetStringReport());
                return false;
            }
            if (result->GetAffectedRows() != objects.size()) {
                TFLEventLog::Error("cannot upsert records into snapshot")("reason", "inconsistency records count")("expected", objects.size())("received", result->GetAffectedRows());
                return false;
            }
            if (!tr->Commit()) {
                TFLEventLog::Error("cannot commit snapshot insertion")("reason", tr->GetStringReport());
                return false;
            }
            return true;
        }

        bool TDBTableObjectsManager::DoPutObjects(const TVector<TMappedObject>& objects, const ui32 snapshotId, const TString& /*userId*/) const {
            TRecordsSet recordsRequest;
            for (auto&& i : objects) {
                recordsRequest.AddRow(i.SerializeToTableRecord());
            }
            auto table = Database->GetTable(TableName + ::ToString(snapshotId));
            if (!table) {
                TFLEventLog::Error("there in no snapshot table")("table_name", TableName + ::ToString(snapshotId));
                return false;
            }
            auto result = table->BulkUpsertRows(recordsRequest, *Database);
            if (!result->IsSucceed()) {
                TFLEventLog::Error("cannot insert records into snapshot");
                return false;
            }
            if (result->GetAffectedRows() != objects.size()) {
                TFLEventLog::Error("cannot add records into snapshot")("reason", "inconsistency records count")("expected", objects.size())("received", result->GetAffectedRows());
                return false;
            }
            return true;
        }

        NCS::NScheme::TScheme TDBTableObjectsManager::GetScheme(const IBaseServer& server) const {
            NCS::NScheme::TScheme result = TBase::GetScheme(server);
            result.Add<TFSVariants>("db_name").SetVariants(server.GetDatabaseNames());
            result.Add<TFSString>("table_name").SetNonEmpty(true);
            return result;
        }

        bool TDBTableObjectsManager::DoRemoveSnapshot(const ui32 snapshotId) const {
            NStorage::ITransaction::TPtr tr = Database->CreateTransaction(false);
            if (!Database->DropTable(TableName + ::ToString(snapshotId))) {
                TFLEventLog::Error("cannot drop table");
                return false;
            }
            if (!tr->Commit()) {
                TFLEventLog::Error("cannot commit snapshot removing")("reason", tr->GetStringReport());
                return false;
            }
            return true;
        }

        bool TDBTableObjectsManager::DoCreateSnapshot(const ui32 snapshotId) const {
            auto gLogging = TFLRecords::StartContext()("table_name", TableName + ::ToString(snapshotId));
            NCS::NStorage::TCreateTableQuery ctRequest(TableName + ::ToString(snapshotId), GetStructure().GetFields());
            if (!Database->CreateTable(ctRequest)) {
                TFLEventLog::Error("cannot create table");
                return false;
            }
            for (auto&& i : GetStructure().GetIndexes()) {
                NCS::NStorage::TCreateIndexQuery qIndex(TableName + ::ToString(snapshotId));
                for (auto&& c : i.GetOrderedFields()) {
                    qIndex.AddColumn(c);
                }
                if (!Database->CreateIndex(qIndex)) {
                    TFLEventLog::Error("cannot create index");
                    return false;
                }
            }
            return true;
        }

        bool TDBAsyncFetcherSelection::DoFetch(TVector<TMappedObject>& result) const {
            if (!QueryResult) {
                TFLEventLog::Error("not initialized query result");
                return false;
            }
            if (!QueryResult->Fetch()->IsSucceed()) {
                TFLEventLog::Error("query failed");
                return false;
            }
            TVector<TMappedObject> resultLocal;
            for (auto&& i : Objects.DetachObjects()) {
                resultLocal.emplace_back(std::move(i));
            }
            std::swap(resultLocal, result);
            return true;
        }

    }
}
