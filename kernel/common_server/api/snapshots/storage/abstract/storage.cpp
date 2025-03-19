#include "storage.h"

namespace NCS {
    namespace NSnapshots {

        bool IObjectsManager::IsActive() const {
            return AtomicGet(Active) == 1;
        }

        bool IObjectsManager::Initialize(const IBaseServer& server) {
            CHECK_WITH_LOG(AtomicCas(&Active, 1, 0));
            return DoInitialize(server);
        }

        bool IObjectsManager::RemoveSnapshot(const ui32 snapshotId) const {
            auto gLogging = TFLRecords::StartContext().Method("RemoveSnapshot");
            if (!IsActive()) {
                TFLEventLog::Error("not started snapshots manager");
                return false;
            }
            return DoRemoveSnapshot(snapshotId);
        }

        bool IObjectsManager::CreateSnapshot(const ui32 snapshotId) const {
            auto gLogging = TFLRecords::StartContext().Method("CreateSnapshot");
            if (!IsActive()) {
                TFLEventLog::Error("not started snapshots manager");
                return false;
            }
            return DoCreateSnapshot(snapshotId);
        }

        bool IObjectsManager::RemoveSnapshotObjects(const TVector<NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const {
            auto gLogging = TFLRecords::StartContext().Method("RemoveSnapshotObjects");
            if (!IsActive()) {
                TFLEventLog::Error("not started snapshots manager");
                return false;
            }
            TMap<TString, TVector<NStorage::TTableRecordWT>> objectsByIndex;
            for (auto&& i : objectIds) {
                NStorage::TTableRecordWT record;
                TMaybe<TIndex> index = Structure.SearchIndex(i, false, &record);
                if (!index) {
                    TFLEventLog::Error("cannot detect index")("raw_record", i.SerializeToJson());
                    return false;
                }
                objectsByIndex[index->GetIndexId()].emplace_back(std::move(record));
            }
            TVector<TMappedObject> resultLocal;
            for (auto&& i : objectsByIndex) {
                TVector<TMappedObject> resultIndex;
                if (!DoRemoveSnapshotObjects(i.second, snapshotId)) {
                    TFLEventLog::Error("cannot fetch objects")("index_id", i.first);
                    return false;
                }
                resultLocal.insert(resultLocal.end(), resultIndex.begin(), resultIndex.end());
            }
            return true;
        }

        bool IObjectsManager::RemoveSnapshotObjectsBySRCondition(const ui32 snapshotId, const TSRCondition& customCondition) const {
            auto gLogging = TFLRecords::StartContext().Method("RemoveSnapshotObjectsBySRCondition");
            if (!IsActive()) {
                TFLEventLog::Error("not started snapshots manager");
                return false;
            }
            if (!DoRemoveSnapshotObjectsBySRCondition(snapshotId, customCondition)) {
                TFLEventLog::Error("cannot remove_snapshot_objects_by_custom_condition");
                return false;
            }
            return true;
        }

        bool IObjectsManager::GetObjects(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId, TVector<TMappedObject>& result) const {
            auto gLogging = TFLRecords::StartContext().Method("GetObjects")("snapshot_id", snapshotId);
            if (!IsActive()) {
                TFLEventLog::Error("not started snapshots manager");
                return false;
            }
            TMap<TString, TVector<NCS::NStorage::TTableRecordWT>> objectsByIndex;
            for (auto&& i : objectIds) {
                NStorage::TTableRecordWT record;
                TMaybe<TIndex> index = Structure.SearchIndex(i, false, &record);
                if (!index) {
                    TFLEventLog::Error("cannot detect index")("raw_record", i.SerializeToJson());
                    return false;
                }
                objectsByIndex[index->GetIndexId()].emplace_back(std::move(record));
            }
            TVector<TMappedObject> resultLocal;
            for (auto&& i : objectsByIndex) {
                TVector<TMappedObject> resultIndex;
                auto fetchResult = DoGetObjects(i.second, snapshotId);
                if (!fetchResult || !fetchResult->Fetch(resultIndex)) {
                    TFLEventLog::Error("cannot fetch objects")("index_id", i.first);
                    return false;
                }
                resultLocal.insert(resultLocal.end(), resultIndex.begin(), resultIndex.end());
            }
            std::swap(result, resultLocal);
            return true;
        }

        bool IObjectsManager::GetAllObjects(const ui32 snapshotId, TVector<TMappedObject>& result, const TObjectsFilter& objectsFilter) const {
            auto gLogging = TFLRecords::StartContext().Method("GetAllObjects")("snapshot_id", snapshotId);
            if (!IsActive()) {
                TFLEventLog::Error("not started snapshots manager");
                return false;
            }
            TVector<TMappedObject> resultLocal;
            if (!DoGetAllObjects(snapshotId, resultLocal, objectsFilter)) {
                TFLEventLog::Error("cannot fetch all objects");
                return false;
            }
            std::swap(result, resultLocal);
            return true;
        }

        ISelectionResult::TPtr IObjectsManager::FetchObjectAsync(const TVector<NCS::NStorage::TTableRecordWT>& objectIds, const ui32 snapshotId) const {
            return DoGetObjects(objectIds, snapshotId);
        }

        bool IObjectsManager::PutObjects(const TVector<TMappedObject>& objects, const ui32 snapshotId, const TString& userId) const {
            auto gLogging = TFLRecords::StartContext().Method("PutObjects");
            if (!IsActive()) {
                TFLEventLog::Error("not started snapshots manager");
                return false;
            }
            TVector<TMappedObject> objectsLocal = objects;
            const TSet<TString> fieldIds = Structure.GetFieldIds();
            for (auto&& i : objectsLocal) {
                i.FilterColumns(fieldIds);
            }
            return DoPutObjects(objectsLocal, snapshotId, userId);
        }

        bool IObjectsManager::UpsertObjects(const TVector<TMappedObject>& objects, const ui32 snapshotId, const TString& userId) const {
            auto gLogging = TFLRecords::StartContext().Method("UpsertObjects");
            if (!IsActive()) {
                TFLEventLog::Error("not started snapshots manager");
                return false;
            }
            TMap<TIndex, TVector<NCS::NSnapshots::TMappedObject>> objectsByIndex;
            for (auto&& i : objects) {
                NCS::NStorage::TTableRecordWT tr = i.GetValues();
                TMaybe<TIndex> index = Structure.SearchIndex(tr, true, nullptr);
                if (!index) {
                    TFLEventLog::Error("cannot detect index")("raw_record", i.SerializeToJson());
                    return false;
                }
                objectsByIndex[*index].emplace_back(Structure.FilterColumns(std::move(tr)));
            }
            for (auto&& i : objectsByIndex) {
                if (!DoUpsertObjects(i.first, i.second, snapshotId, userId)) {
                    TFLEventLog::Error("cannot fetch objects")("index_id", i.first.GetIndexId());
                    return false;
                }
            }
            return true;
        }

        bool IObjectsManager::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::ReadObject(jsonInfo, "structure", Structure, true)) {
                return false;
            }
            return DoDeserializeFromJson(jsonInfo);
        }

        bool TObjectsManagerContainer::Start(const IBaseServer& server) {
            if (!Object) {
                TFLEventLog::Error("object not constructed for start snapshot objects manager");
                return false;
            }
            return Object->Initialize(server);
        }

    }
}
