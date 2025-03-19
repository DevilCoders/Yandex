#pragma once
#include <kernel/common_server/api/common.h>
#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/library/storage/selection/iterator.h>
#include "db_owner.h"
#include "db_entities_history.h"

template <class TBaseClass>
class TDBDirectOperatorBase: public TBaseClass {
private:
    using TBase = TBaseClass;
protected:
    using TBase::Database;
    using TBase::HistoryManager;
public:
    using TEntity = typename TBaseClass::TEntity;
    using TIdType = typename TBaseClass::TEntity::TId;
    using TBase::TBase;

    bool AddRecord(const NStorage::TTableRecord& record, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr, const NStorage::TTableRecord* condition = nullptr) const {
        TRecordsSet rSet;
        NStorage::TTableRecord recordLocal = record;
        rSet.AddRow(std::move(recordLocal));
        return AddRecords(rSet, userId, session, dbObjectsResult, condition);
    }

    bool AddRecord(NStorage::TTableRecord&& record, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult, const NStorage::TTableRecord* condition = nullptr) const {
        TRecordsSet rSet;
        rSet.AddRow(std::move(record));
        return AddRecords(rSet, userId, session, dbObjectsResult, condition);
    }

    bool AddRecord(NStorage::TTableRecord&& record, const TString& userId, NCS::TEntitySession& session, TMaybe<TEntity>* dbObjectResult, const NStorage::TTableRecord* condition = nullptr) const {
        TRecordsSet rSet;
        rSet.AddRow(std::move(record));
        TVector<TEntity> entities;
        if (!AddRecords(rSet, userId, session, &entities, condition)) {
            return false;
        }
        CHECK_WITH_LOG(entities.size() <= 1);
        if (entities.size() && dbObjectResult) {
            *dbObjectResult = std::move(entities.front());
        }
        return true;
    }

    bool AddRecords(const TRecordsSet& records, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr, const NStorage::TTableRecord* condition = nullptr) const {
        auto gLogging = TFLRecords::StartContext().Method("AddRecords")("table_name", TEntity::GetTableName());
        NStorage::ITableAccessor::TPtr table = Database->GetTable(TEntity::GetTableName());
        NStorage::TObjectRecordsSet<TEntity> dbObjects;
        if (condition) {
            if (!table->AddIfNotExists(records, session.GetTransaction(), *condition, &dbObjects)) {
                return false;
            }
        } else {
            if (!table->AddRows(records, session.GetTransaction(), "", &dbObjects)) {
                return false;
            }
        }
        if (!condition && dbObjects.size() != records.size()) {
            session.Error("Incorrect records", ESessionResult::InconsistencySystem)("expected_count", records.size())("parsed_count", dbObjects.size());
            return false;
        }
        if (!HistoryManager->AddHistory(dbObjects.GetObjects(), userId, EObjectHistoryAction::Add, session)) {
            return false;
        }
        if (dbObjectsResult) {
            *dbObjectsResult = dbObjects.DetachObjects();
        }
        return true;
    }

    bool AddObjects(const TVector<TEntity>& objects, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr, const NStorage::TTableRecord* condition = nullptr) const {
        TRecordsSet records;
        for (auto&& i : objects) {
            records.AddRow(i.SerializeToTableRecord());
        }
        return AddRecords(records, userId, session, dbObjectsResult, condition);
    }

    bool RestoreObjectsBySRCondition(TVector<TEntity>& result, const TSRCondition& condition, NCS::TEntitySession& session) const {
        auto gLogging = TFLRecords::StartContext().Method("RestoreObjectsBySRCondition")("table_name", TEntity::GetTableName());
        NStorage::TObjectRecordsSet<TEntity> records;
        TSRSelect select(TEntity::GetTableName(), &records);
        select.SetCondition(condition);
        if (!session.ExecRequest(select)) {
            return false;
        }
        result = records.DetachObjects();
        return true;
    }

    bool RestoreObjectsBySRCondition(TVector<TEntity>& result, const TSRCondition& condition, NCS::TEntitySession* session = nullptr) const {
        if (session) {
            return RestoreObjectsBySRCondition(result, condition, *session);
        } else {
            auto sessionLocal = TBase::BuildNativeSession(true);
            return RestoreObjectsBySRCondition(result, condition, sessionLocal);
        }
    }

    bool CleanNotUsed(const TString& usageTableName, const TString& usageObjectFieldId, const TString& userId, NCS::TEntitySession& session) const {
        TSRCondition srCondition;
        TSRSelect& srSelect = srCondition.Ret<TSRBinary>(ESRBinary::NotIn)
            .InitLeft<TSRFieldName>(TEntity::GetIdFieldName()).template RetRight<TSRSelect>(usageTableName);
        srSelect.InitFields<TSRFieldName>(usageObjectFieldId);
        return RemoveObjectsBySRCondition(srCondition, userId, session);
    }

    bool RemoveObjectsBySRCondition(const TSRCondition& condition, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr, const i32 expectedCount = -1) const {
        return RemoveRecords(condition, userId, session, dbObjectsResult, expectedCount);
    }

    bool RestoreObjectBySRCondition(const TSRCondition& condition, TMaybe<TEntity>& result, NCS::TEntitySession& session) const {
        TVector<TEntity> objects;
        if (!RestoreObjectsBySRCondition(objects, condition, session)) {
            return false;
        }
        if (objects.size() > 1) {
            session.SetErrorInfo("RestoreObjectByCondition", "records count on restore object > 1", ESessionResult::InconsistencySystem);
            return false;
        }
        if (objects.empty()) {
            return true;
        }
        result = objects.front();
        return true;
    }

    bool RestoreAllObjects(TVector<TEntity>& objects, NCS::TEntitySession& session) const {
        return RestoreObjectsBySRCondition(objects, TSRCondition(), session);
    }

    bool RestoreAllObjects(TVector<TEntity>& objects, const bool fromMaster = false) const {
        auto session = TBase::BuildNativeSession(!fromMaster);
        return RestoreObjectsBySRCondition(objects, TSRCondition(), session);
    }

    template <class TEntityId>
    bool RestoreAllObjects(TMap<TEntityId, TEntity>& objects, NCS::TEntitySession& session) const {
        TVector<TEntity> vObjects;
        if (!RestoreAllObjects(vObjects, session)) {
            return false;
        }
        for (auto&& i : vObjects) {
            auto id = i.GetInternalId();
            objects.emplace(id, std::move(i));
        }
        return true;
    }

    bool RestoreObject(const TIdType& id, TMaybe<TEntity>& result, NCS::TEntitySession& session) const {
        return RestoreObjectBySRCondition(TSRCondition().Init<TSRBinary>(TEntity::GetIdFieldName(), id), result, session);
    }

    TMaybe<TEntity> RestoreObject(const TIdType& id, NCS::TEntitySession& session) const {
        TMaybe<TEntity> result;
        if (!RestoreObject(id, result, session)) {
            return Nothing();
        }
        return result;
    }

    bool RestoreObjects(const TSet<TIdType>& ids, TVector<TEntity>& result, NCS::TEntitySession& session) const {
        if (ids.empty()) {
            return true;
        }
        TSRCondition condition;
        condition.Init<TSRBinary>(TEntity::GetIdFieldName(), ids);
        return RestoreObjectsBySRCondition(result, condition, session);
    }

    bool RestoreObjects(const TSet<TIdType>& ids, TMap<TIdType, TEntity>& result, NCS::TEntitySession& session) const {
        if (ids.empty()) {
            return true;
        }
        TSRCondition condition;
        condition.Init<TSRBinary>(TEntity::GetIdFieldName(), ids);
        TVector<TEntity> resultObjects;
        if (!RestoreObjectsBySRCondition(resultObjects, condition, session)) {
            return false;
        }
        TMap<TIdType, TEntity> resultLocal;
        for (auto&& i : resultObjects) {
            const TIdType id = i.GetInternalId();
            resultLocal.emplace(id, std::move(i));
        }
        std::swap(result, resultLocal);
        return true;
    }

    bool RemoveOtherObjects(const TSet<TIdType>& ids, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr, const i32 expectedCount = -1) const {
        if (ids.empty()) {
            return true;
        }
        TSRCondition condition;
        condition.InitObject<TSRBinary>(TEntity::GetIdFieldName(), ids, ESRBinary::NotIn);
        return RemoveObjectsBySRCondition(condition, userId, session, dbObjectsResult, expectedCount);
    }

    bool UpdateRecord(const TSRCondition& update, const TSRCondition& condition, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult, const i32 expectedCount = 1, const EObjectHistoryAction action = EObjectHistoryAction::UpdateData, const bool storeHistory = true) const {
        const auto gLogging = TFLRecords::StartContext().Method("UpdateRecord")("table_name", TEntity::GetTableName());

        NStorage::TObjectRecordsSet<TEntity> dbObjects;
        TSRUpdate srUpdate(TEntity::GetTableName(), &dbObjects);
        srUpdate.SetUpdate(update).SetCondition(condition);
        if (!session.ExecRequest(srUpdate)) {
            return false;
        }
        if (expectedCount >= 0 && dbObjects.size() != (size_t)expectedCount) {
            session.Error("UpdateRecordInconsistency")("expectation", expectedCount)("reality", dbObjects.size())("condition", condition.DebugString());
            return false;
        }
        if (storeHistory && !HistoryManager->AddHistory(dbObjects.GetObjects(), userId, action, session)) {
            return false;
        }
        if (dbObjectsResult) {
            *dbObjectsResult = dbObjects.DetachObjects();
        }
        return true;
    }

    bool UpdateRecord(const NCS::NStorage::TTableRecord& update, const NCS::NStorage::TTableRecord& condition, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult, const i32 expectedCount = 1, const EObjectHistoryAction action = EObjectHistoryAction::UpdateData, const bool storeHistory = true) const {
        const auto gLogging = TFLRecords::StartContext().Method("UpdateRecord")("table_name", TEntity::GetTableName());
        NStorage::ITableAccessor::TPtr table = Database->GetTable(TEntity::GetTableName());

        NStorage::TObjectRecordsSet<TEntity> dbObjects;
        auto result = table->UpdateRow(condition, update, session.GetTransaction(), &dbObjects);
        if (!result->IsSucceed()) {
            session.Error("UpdateRecordFailed", ESessionResult::TransactionProblem)("message", session.GetTransaction()->GetStringReport());
            return false;
        }
        if (expectedCount >= 0 && dbObjects.size() != (size_t)expectedCount) {
            session.Error("UpdateRecordInconsistency")("expectation", expectedCount)("reality", dbObjects.size())("condition", condition);
            return false;
        }
        if (storeHistory && !HistoryManager->AddHistory(dbObjects.GetObjects(), userId, action, session)) {
            return false;
        }
        if (dbObjectsResult) {
            *dbObjectsResult = dbObjects.DetachObjects();
        }
        return true;
    }

    bool UpdateRecords(const TVector<NStorage::TTableRecord>& updates, const TVector<NStorage::TTableRecord>& conditions, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult, const i32 expectedCount = -1, const EObjectHistoryAction action = EObjectHistoryAction::UpdateData, const bool storeHistory = true) const {
        const auto gLogging = TFLRecords::StartContext().Method("UpdateRecords")("table_name", TEntity::GetTableName());
        NStorage::ITableAccessor::TPtr table = Database->GetTable(TEntity::GetTableName());

        NStorage::TObjectRecordsSet<TEntity> dbObjects;
        auto result = table->UpdateRows(conditions, updates, session.GetTransaction(), &dbObjects);
        if (!result->IsSucceed()) {
            session.Error("UpdateRecordFailed", ESessionResult::TransactionProblem)("message", session.GetTransaction()->GetStringReport());
            return false;
        }
        if (expectedCount >= 0 && dbObjects.size() != (size_t)expectedCount) {
            session.Error("UpdateRecordInconsistency")("expectation", expectedCount)("reality", dbObjects.size())("conditions", conditions);
            return false;
        }
        if (storeHistory && !HistoryManager->AddHistory(dbObjects.GetObjects(), userId, action, session)) {
            return false;
        }
        if (dbObjectsResult) {
            *dbObjectsResult = dbObjects.DetachObjects();
        }
        return true;
    }

    bool UpsertRecord(const NStorage::TTableRecord& update, const NStorage::TTableRecord& condition, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult, bool* isUpdate = nullptr) const {
        const auto gLogging = TFLRecords::StartContext().Method("UpsertRecord")("table_name", TEntity::GetTableName());
        NStorage::ITableAccessor::TPtr table = Database->GetTable(TEntity::GetTableName());

        NStorage::TObjectRecordsSet<TEntity> dbObjects;
        bool isUpdateLocal = false;
        auto result = table->Upsert(update, session.GetTransaction(), condition, &isUpdateLocal, &dbObjects);
        if (!result->IsSucceed()) {
            session.Error("UpsertRecordFailed", ESessionResult::TransactionProblem)("message", session.GetTransaction()->GetStringReport());
            return false;
        }
        if (!HistoryManager->AddHistory(dbObjects.GetObjects(), userId, isUpdateLocal ? EObjectHistoryAction::UpdateData : EObjectHistoryAction::Add, session)) {
            return false;
        }
        if (dbObjectsResult) {
            *dbObjectsResult = dbObjects.DetachObjects();
        }
        if (isUpdate) {
            *isUpdate = isUpdateLocal;
        }
        return true;
    }

    bool UpsertRecordNative(const NStorage::TTableRecord& update, const TSet<TString>& uniqueFieldIds, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr, bool* isUpdateExt = nullptr) const {
        const auto gLogging = TFLRecords::StartContext().Method("UpsertRecord")("table_name", TEntity::GetTableName());
        NStorage::ITableAccessor::TPtr table = Database->GetTable(TEntity::GetTableName());

        NStorage::TObjectRecordsSet<TEntity> dbObjects;
        TSRInsert reqInsert(TEntity::GetTableName(), &dbObjects);
        reqInsert.FillRecords({ update });
        reqInsert.MutableUniqueFieldIds().InitFromSet(uniqueFieldIds);
        reqInsert.SetConflictPolicy(TSRInsert::EConflictPolicy::Update);
        if (!session.ExecRequest(reqInsert)) {
            return false;
        }
        if (!HistoryManager->AddHistory(dbObjects.GetObjects(), userId, EObjectHistoryAction::UpsertData, session)) {
            return false;
        }
        if (dbObjectsResult) {
            *dbObjectsResult = dbObjects.DetachObjects();
        }
        if (isUpdateExt) {
            *isUpdateExt = false;
        }
        return true;
    }

    bool RemoveRecord(const NStorage::TTableRecord& condition, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr, const i32 expectedCount = -1) const {
        TRecordsSet records;
        NStorage::TTableRecord condLocal = condition;
        records.AddRow(std::move(condLocal));
        return RemoveRecords(records, userId, session, dbObjectsResult, expectedCount);
    }

    bool RemoveObjects(const TSet<TIdType>& ids, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr) const {
        return RemoveObjectsByCustomIds(TEntity::GetIdFieldName(), ids, userId, session, dbObjectsResult);
    }

    bool RemoveRecords(const TRecordsSet& records, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr, const i32 expectedCount = -1, const EObjectHistoryAction action = EObjectHistoryAction::Remove) const {
        const auto gLogging = TFLRecords::StartContext().Method("RemoveRecords")("table_name", TEntity::GetTableName());

        NStorage::TObjectRecordsSet<TEntity> dbObjects;
        TSRDelete srDelete(TEntity::GetTableName(), &dbObjects);
        srDelete.RetCondition<TSRMulti>().FillRecords(records.GetRecords());
        if (!session.ExecRequest(srDelete)) {
            return false;
        }
        if (expectedCount >= 0 && dbObjects.size() != (size_t)expectedCount) {
            session.Error("RemoveRecordsInconsistency")("expectation", expectedCount)("reality", dbObjects.size());
            return false;
        }
        if (!HistoryManager->AddHistory(dbObjects.GetObjects(), userId, action, session)) {
            return false;
        }
        if (dbObjectsResult) {
            *dbObjectsResult = dbObjects.DetachObjects();
        }
        return true;
    }

    bool RemoveRecords(const TSRCondition& condition, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr, const i32 expectedCount = -1, const EObjectHistoryAction action = EObjectHistoryAction::Remove) const {
        const auto gLogging = TFLRecords::StartContext().Method("RemoveRecords")("table_name", TEntity::GetTableName());
        NStorage::TObjectRecordsSet<TEntity> dbObjects;
        TSRDelete reqDelete(TEntity::GetTableName(), &dbObjects);
        reqDelete.SetCondition(condition);

        if (!session.ExecRequest(reqDelete)) {
            return false;
        }

        if (expectedCount >= 0 && dbObjects.GetObjects().size() != (size_t)expectedCount) {
            session.Error("RemoveRecordInconsistency")("expectation", expectedCount)("reality", dbObjects.size())("condition", reqDelete.DebugString());
            return false;
        }

        if (!HistoryManager->AddHistory(dbObjects.GetObjects(), userId, action, session)) {
            return false;
        }
        if (dbObjectsResult) {
            *dbObjectsResult = dbObjects.DetachObjects();
        }
        return true;
    }

    template <class TIdTypeLocal>
    bool RemoveObjectsByCustomIds(const TString& fieldName, const TSet<TIdTypeLocal>& ids, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult = nullptr) const {
        if (ids.empty()) {
            return true;
        }
        TSRCondition condition;
        condition.Init<TSRBinary>(fieldName, ids);
        return RemoveRecords(condition, userId, session, dbObjectsResult);
    }

    template <class TIdTypeLocal>
    bool RestoreObjectsByCustomIds(const TString& fieldName, const TSet<TIdTypeLocal>& ids, TVector<TEntity>& result, NCS::TEntitySession& session) const {
        if (ids.empty()) {
            return true;
        }
        TSRCondition condition;
        condition.Init<TSRBinary>(fieldName, ids);
        return RestoreObjectsBySRCondition(result, condition, session);
    }

};

template <class TBaseClass>
class TDBDirectOperator: public TBaseClass, public virtual IDirectObjectsOperator<typename TBaseClass::TEntity> {
private:
    using TBaseOperator = IDirectObjectsOperator<typename TBaseClass::TEntity>;
    using TBase = TBaseClass;
    using TBase::BuildNativeSession;
public:
    using TEntity = typename TBaseClass::TEntity;
protected:
    using TBase::HistoryConfig;
    using TBase::HistoryContext;
    using TBase::HistoryManager;
    virtual bool DoDirectRestoreAllObjects(TVector<TEntity>& result, NCS::TEntitySession& session) const override {
        return TBase::RestoreAllObjects(result, session);
    }
    virtual bool DoDirectRestoreObjects(const TSet<typename TEntity::TId>& ids, TVector<TEntity>& result, NCS::TEntitySession& session) const override {
        return TBase::RestoreObjects(ids, result, session);
    }
    virtual bool DoRestoreHistory(TVector<TObjectEvent<TEntity>>& result, NCS::TEntitySession& session) const override {
        return HistoryManager->RestoreEventsByCondition(TSRCondition(), result, session);
    }

    virtual bool DoRestoreHistoryEvent(const ui64 historyEventId, TMaybe<TObjectEvent<TEntity>>& result, NCS::TEntitySession& session) const override {
        TVector<TObjectEvent<TEntity>> events;
        if (!HistoryManager->RestoreEventsById({ historyEventId }, events, session)) {
            return false;
        }
        if (events.size() == 0) {
            result.Clear();
            return true;
        } else if (events.size() == 1) {
            result = std::move(events.front());
            return true;
        } else {
            TFLEventLog::Error("incorrect restored events count")("event_id", historyEventId)("count", events.size());
            return false;
        }
        return true;
    }

    bool UpsertObjectSimple(const TEntity& object, const TString& userId, NCS::TEntitySession& session, TVector<TEntity>* dbObjectsResult, bool* isUpdate = nullptr) const {
        auto trUpdate = object.SerializeToTableRecord();
        NStorage::TTableRecord trCondition;
        trCondition.Set(TEntity::GetIdFieldName(), object.GetInternalId());
        return TBase::UpsertRecord(trUpdate, trCondition, userId, session, dbObjectsResult, isUpdate);
    }

    virtual bool DoUpsertObjectForce(const TEntity& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdate, TEntity* result) const override {
        TVector<TEntity> objects;
        if (!UpsertObjectSimple(object, userId, session, &objects, isUpdate)) {
            return false;
        } else if (objects.size() != 1) {
            session.Error("incorrect records count affected by upsert")("size", objects.size());
            return false;
        } else {
            if (result) {
                *result = std::move(objects.front());
            }
            return true;
        }
    }

    virtual bool DoUpsertObject(const TEntity& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdate, TEntity* result) const override {
        return TBaseOperator::UpsertObjectForce(object, userId, session, isUpdate, result);
    }

public:
    using TBase::TBase;

    virtual NCS::TEntitySession BuildNativeSession(const bool readOnly = false, const bool repeatableRead = false) const override
    {
        return TBase::BuildNativeSession(readOnly, repeatableRead);
    }

};

template <class TObject>
using TDBDirectOperatorDefault = TDBDirectOperator<TDBDirectOperatorBase<NCS::TDBHistoryOwner<TDBEntitiesHistoryManager<TObject>>>>;
