#pragma once
#include "manager.h"
#include "cache.h"
#include "db_entities_history.h"
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/algorithm/container.h>
#include <kernel/common_server/common/scheme.h>

template <class TId>
class TRevisionedId {
private:
    CS_ACCESS(TRevisionedId, TId, Id, TId());
    CS_ACCESS(TRevisionedId, ui64, Revision, 0);
public:
    TRevisionedId() = default;

    TRevisionedId(const TId& id, const ui64 revision)
        : Id(id)
        , Revision(revision)
    {

    }

    bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        if (!TJsonProcessor::Read(jsonInfo, "id", Id)) {
            return false;
        }
        if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
            return false;
        }
        return true;
    }

    bool operator<(const TRevisionedId& item) const {
        return Id < item.Id || (Id == item.Id && item.Revision < Revision);
    }
};

using TRevisionedStrId = TRevisionedId<TString>;

template <class TId>
class TRevisionedIds {
private:
    using TRevisionedId = ::TRevisionedId<TId>;
    CSA_DEFAULT(TRevisionedIds, TSet<TRevisionedId>, RevisionedIds);
public:
    bool Contains(const TId& id, const ui64 revision) const {
        return RevisionedIds.contains(TRevisionedId(id, revision));
    }

    bool DeserializeFromJson(const NJson::TJsonValue& jsonIds) {
        const NJson::TJsonValue::TArray* arr;
        if (!jsonIds.GetArrayPointer(&arr)) {
            return false;
        }
        for (auto&& i : *arr) {
            TRevisionedId id;
            if (!id.DeserializeFromJson(i)) {
                return false;
            }
            RevisionedIds.emplace(id);
        }
        return true;
    }

    NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue result = NJson::JSON_ARRAY;
        for (auto&& i : RevisionedIds) {
            result.AppendValue(i.SerializeToJson());
        }
        return result;
    }

    TSet<TId> GetIdsSet() const {
        TSet<TId> result;
        for (auto&& i : RevisionedIds) {
            result.emplace(i.GetId());
        }
        return result;
    }

    bool CheckUniqueIds() const {
        TSet<TId> result;
        for (auto&& i : RevisionedIds) {
            if (!result.emplace(i.GetId()).second) {
                return false;
            }
        }
        return true;
    }
};

using TRevisionedStrIds = TRevisionedIds<TString>;

class TDBEntitiesManagerConfig {
private:
    CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
    CSA_DEFAULT(TDBEntitiesManagerConfig, TString, DBName);
public:

    TDBEntitiesManagerConfig() = default;
    explicit TDBEntitiesManagerConfig(const THistoryConfig& hConfig)
        : HistoryConfig(hConfig)
    {

    }

    void Init(const TYandexConfig::Section* section) {
        DBName = section->GetDirectives().Value("DBName", DBName);
        auto children = section->GetAllChildren();
        auto it = children.find("History");
        if (it != children.end()) {
            HistoryConfig.Init(it->second);
        }
    }

    void ToString(IOutputStream& os) const {
        if (!!DBName) {
            os << "DBName: " << DBName << Endl;
        }
        os << "<History>" << Endl;
        HistoryConfig.ToString(os);
        os << "</History>" << Endl;
    }
};

template <class T>
TVector<T> FilterRevision(const TVector<T>& objects, const ui64 revision) {
    TVector<T> result;
    for (auto&& i : objects) {
        if (!i.GetSnapshotRevision() || revision == i.GetSnapshotRevision() || !revision) {
            result.emplace_back(i);
        }
    }
    return result;
}

class TSnapshotConstructor {
private:
    const TString SnapshotId;
    const TString UserId;
    const ui64 PredSnapshotRevision;
    const ui64 NextSnapshotRevision;
    NCS::TEntitySession& Session;
    RTLINE_ACCEPTOR(TSnapshotConstructor, SnapshotIdField, TString, "snapshot_id");
public:
    TSnapshotConstructor(const ui64 baseRevision, const ui64 nextRevision, const TString& snapshotId, const TString& userId, NCS::TEntitySession& session)
        : SnapshotId(snapshotId)
        , UserId(userId)
        , PredSnapshotRevision(baseRevision)
        , NextSnapshotRevision(nextRevision)
        , Session(session)
    {


    }

    template <class TDBObject, class TManager>
    bool AddRevisionedObjects(const TManager& manager, const TVector<TDBObject>& objects, TVector<TDBObject>* dbObjectsResult) const {
        TRecordsSet records;
        for (auto&& i : objects) {
            NStorage::TTableRecord tr = i.SerializeToTableRecord();
            tr.ForceSet("snapshot_revision", NextSnapshotRevision);
            tr.ForceSet(SnapshotIdField, SnapshotId);
            records.AddRow(std::move(tr));
        }
        return manager.AddRecords(records, UserId, Session, dbObjectsResult);
    }

    template <class TDBObject, class TManager>
    bool UpdateRevisionedObjects(const TManager& manager, const TVector<TDBObject>& objects, TVector<TDBObject>* dbObjectsResult) const {
        for (auto&& i : objects) {
            NStorage::TTableRecord trUpdate = i.SerializeToTableRecord();
            trUpdate.ForceSet("snapshot_revision", NextSnapshotRevision);

            NStorage::TTableRecord trCondition = i.GetObjectIdRecord();
            trCondition.Set("snapshot_revision", PredSnapshotRevision);

            if (!manager.UpdateRecord(trUpdate, trCondition, UserId, Session, dbObjectsResult)) {
                return false;
            }
        }
        return true;
    }
};

template <class TObjectContainer>
class TDefaultConditionConstructor {
public:
    static NStorage::TTableRecord BuildCondition(const typename TObjectContainer::TId& id) {
        NStorage::TTableRecord trCondition;
        trCondition.Set(TObjectContainer::GetIdFieldName(), id);
        return trCondition;
    }

    static NStorage::TTableRecord BuildCondition(const TObjectContainer& object) {
        return BuildCondition(object.GetInternalId());
    }
};

template <class TObjectContainer>
class TDBEntitiesManager: public TDBEntitiesCache<TObjectContainer, TDBRevisionedEntitiesHistoryManager<TObjectContainer>>, public virtual IDBEntitiesManager<TObjectContainer> {
    using TBase = TDBEntitiesCache<TObjectContainer, TDBRevisionedEntitiesHistoryManager<TObjectContainer>>;
    using TConditionConstructor = TDefaultConditionConstructor<TObjectContainer>;
protected:
    using TBase::HistoryCacheDatabase;
    using TBase::HistoryManager;
    using TBase::Objects;
    using TBase::MutexCachedObjects;
    bool UpsertObjectImpl(const TObjectContainer& object, const TString& userId, NCS::TEntitySession& session, const bool force, bool* isUpdateExt, TObjectContainer* resultObject) const {
        if (!object) {
            session.SetErrorInfo("incorrect user data", "upsert object into " + TObjectContainer::GetTableName(), ESessionResult::DataCorrupted);
            return false;
        }

        if (!OnBeforeUpsertObject(object, userId, session)) {
            return false;
        }
        NStorage::TTableRecord trUpdate = object.SerializeToTableRecord();

        auto table = HistoryCacheDatabase->GetTable(TObjectContainer::GetTableName());

        NStorage::TObjectRecordsSet<TObjectContainer> records;
        bool isUpdate = false;
        auto revision = object.GetRevisionMaybe();
        if (force) {
            revision = Nothing();
        }
        switch (table->UpsertWithRevision(trUpdate, { TObjectContainer::GetIdFieldName() }, revision, "revision", session.GetTransaction(), &records, force)) {
            case NStorage::ITableAccessor::EUpdateWithRevisionResult::IncorrectRevision:
                session.SetErrorInfo("upsert_object", "incorect_revision", ESessionResult::InconsistencyUser);
                return false;
            case NStorage::ITableAccessor::EUpdateWithRevisionResult::Failed:
                session.SetErrorInfo("upsert_object", session.GetTransaction()->GetStringReport(), ESessionResult::TransactionProblem);
                return false;
            case NStorage::ITableAccessor::EUpdateWithRevisionResult::Updated:
                isUpdate = true;
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
            case NStorage::ITableAccessor::EUpdateWithRevisionResult::Inserted:
                if (records.size() != 1) {
                    session.SetErrorInfo("upsert_object", "cannot parse new records", ESessionResult::TransactionProblem);
                    return false;
                }
                break;
        }

        if (!HistoryManager->AddHistory(records.GetObjects(), userId, isUpdate ? EObjectHistoryAction::UpdateData : EObjectHistoryAction::Add, session)) {
            return false;
        }
        CHECK_WITH_LOG(records.GetObjects().size() == 1) << records.GetObjects().size() << Endl;
        if (!OnAfterUpsertObject(records.GetObjects().front(), userId, session)) {
            return false;
        }
        if (resultObject) {
            *resultObject = records.GetObjects().front();
        }
        if (isUpdateExt) {
            *isUpdateExt = isUpdate;
        }
        return true;
    }

    virtual bool DoUpsertObjectForce(const TObjectContainer& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdate, TObjectContainer* result) const override {
        return UpsertObjectImpl(object, userId, session, true, isUpdate, result);
    }

    virtual bool DoUpsertObject(const TObjectContainer& object, const TString& userId, NCS::TEntitySession& session, bool* isUpdate, TObjectContainer* result) const override {
        return UpsertObjectImpl(object, userId, session, false, isUpdate, result);
    }

public:
    TAtomicSharedPtr<TObjectEvent<TObjectContainer>> GetRevisionedEvent(const typename TObjectContainer::TId& id, const ui64 revision) const {
        return HistoryManager->GetRevisionedEventWithId(revision, id);
    }

    TAtomicSharedPtr<TObjectEvent<TObjectContainer>> GetLastEvent(const typename TObjectContainer::TId& id, const TInstant until, const bool include) const {
        return HistoryManager->GetLastEvent(id, until, include);
    }

    virtual bool GetCustomEntities(TVector<TObjectContainer>& objects, const TSet<typename TObjectContainer::TId>& ids) const override {
        return TBase::GetCustomObjects(objects, ids);
    }

    TMaybe<TObjectContainer> GetMostActualObject(const typename TObjectContainer::TId& id, const TInstant until, const bool include) const {
        if (until) {
            return HistoryManager->GetMostActualObject(id, until, include);
        } else {
            return TBase::GetCustomObject(id);
        }
    }

    TMaybe<TObjectContainer> GetRevisionedObject(const typename TObjectContainer::TId& id, const ui64 revision) const {
        if (revision) {
            auto result = GetRevisionedEvent(id, revision);
            if (!result) {
                return Nothing();
            } else {
                return *result;
            }
        } else {
            return TBase::GetCustomObject(id);
        }
    }

    virtual NStorage::TTableRecord BuildCondition(const typename TObjectContainer::TId& id) const final {
        return TConditionConstructor::BuildCondition(id);
    }

    virtual NStorage::TTableRecord BuildCondition(const TObjectContainer& object) const final {
        return TConditionConstructor::BuildCondition(object);
    }

    virtual bool GetObjects(TVector<TObjectContainer>& objects) const override {
        TMap<typename TObjectContainer::TId, TObjectContainer> objectsMap;
        {
            TReadGuard rg(MutexCachedObjects);
            objectsMap = Objects;
        }
        TVector<TObjectContainer> result;
        for (auto&& [_, i] : objectsMap) {
            result.emplace_back(std::move(i));
        }
        std::swap(result, objects);
        return true;
    }

    virtual bool GetObjects(TMap<typename TObjectContainer::TId, TObjectContainer>& objects, const TInstant reqActuality = TInstant::Zero()) const override {
        if (!TBase::RefreshCache(reqActuality)) {
            return false;
        }
        TReadGuard rg(MutexCachedObjects);
        objects = Objects;
        return true;
    }

    TSet<TString> GetObjectNames() const {
        TSet<TString> result;
        TReadGuard rg(MutexCachedObjects);
        for (auto&& i : Objects) {
            result.emplace(i.first);
        }
        return result;
    }

    template <class TConfig>
    TDBEntitiesManager(const IHistoryContext& context, const TConfig& config)
        : TBase(context, config.GetHistoryConfig()) {
    }

    template <class TConfig>
    TDBEntitiesManager(THolder<IHistoryContext>&& context, const TConfig& config)
        : TBase(std::move(context), config.GetHistoryConfig()) {
    }

    TDBEntitiesManager(const IHistoryContext& context, const THistoryConfig& config)
        : TBase(context, config) {
    }

    TDBEntitiesManager(THolder<IHistoryContext>&& context, const THistoryConfig& config)
        : TBase(std::move(context), config) {
    }

    template <class TConfig>
    TDBEntitiesManager(NStorage::IDatabase::TPtr db, const TConfig& config)
        : TBase(db, config.GetHistoryConfig()) {
    }

    TDBEntitiesManager(NStorage::IDatabase::TPtr db, const THistoryConfig& config)
        : TBase(db, config) {
    }

    bool RestoreObject(const typename TObjectContainer::TId& id, TMaybe<TObjectContainer>& object, NCS::TEntitySession& session) const {
        NStorage::TObjectRecordsSet<TObjectContainer> records;
        TSRSelect select(TObjectContainer::GetTableName(), &records);
        select.RetCondition<TSRMulti>().FillBinary(BuildCondition(id));
        if (!session.ExecRequest(select)) {
            return false;
        }
        if (records.size() > 1) {
            session.Error("restore object failed through object ambiguity")("id", id);
            return false;
        } else if (records.size() == 1) {
            object = records.front();
        }
        return true;
    }

    virtual bool RemoveObject(const TSet<typename TObjectContainer::TId>& ids, const TString& userId, NCS::TEntitySession& session) const override {
        if (ids.empty()) {
            return true;
        }
        auto gLogging = TFLRecords::StartContext().Method("Entities::RemoveObject")("table_name", TObjectContainer::GetTableName());
        TVector<NCS::NStorage::TTableRecord> idRecords;
        for (auto&& id : ids) {
            if (!OnBeforeRemoveObject(id, userId, session)) {
                return false;
            }
            idRecords.emplace_back(BuildCondition(id));
        }
        NStorage::TObjectRecordsSet<TObjectContainer> records;
        TSRDelete srDelete(TObjectContainer::GetTableName(), &records);
        srDelete.RetCondition<TSRMulti>().FillRecords(idRecords);
        if (!session.ExecRequest(srDelete)) {
            return false;
        }
        if (records.size() > ids.size()) {
            session.Error("ambiguous objects for remove")("expected", ids.size())("actual", records.size());
            return false;
        }
        if (!HistoryManager->AddHistory(records.GetObjects(), userId, EObjectHistoryAction::Remove, session)) {
            return false;
        }
        for (auto&& id : ids) {
            if (!OnAfterRemoveObject(id, userId, session)) {
                return false;
            }
        }

        return true;
    }

    virtual bool RemoveObject(const typename TObjectContainer::TId& id, const ui32 revision, const TString& userId, NCS::TEntitySession& session) const override {
        if (!OnBeforeRemoveObject(id, userId, session)) {
            return false;
        }
        NStorage::TObjectRecordsSet<TObjectContainer> records;
        TSRDelete srDelete(TObjectContainer::GetTableName(), &records);
        auto& srMulti = srDelete.RetCondition<TSRMulti>().FillBinary(BuildCondition(id));
        srMulti.template InitNode<TSRBinary>("revision", revision);
        if (!session.ExecRequest(srDelete)) {
            return false;
        }
        if (records.size() != 1) {
            session.SetErrorInfo("Incorrect objects count (" + TObjectContainer::GetTableName() + ")", "remove", ESessionResult::InconsistencySystem);
            return false;
        }
        if (!HistoryManager->AddHistory(records.GetObjects(), userId, EObjectHistoryAction::Remove, session)) {
            return false;
        }
        if (!OnAfterRemoveObject(id, userId, session)) {
            return false;
        }
        return true;
    }

    virtual bool OnBeforeUpsertObject(const TObjectContainer& object, const TString& userId, NCS::TEntitySession& session) const {
        Y_UNUSED(object);
        Y_UNUSED(userId);
        Y_UNUSED(session);
        return true;
    }

    virtual bool OnAfterUpsertObject(const TObjectContainer& object, const TString& userId, NCS::TEntitySession& session) const {
        Y_UNUSED(object);
        Y_UNUSED(userId);
        Y_UNUSED(session);
        return true;
    }

    virtual bool OnBeforeRemoveObject(const typename TObjectContainer::TId& objectId, const TString& userId, NCS::TEntitySession& session) const {
        Y_UNUSED(objectId);
        Y_UNUSED(userId);
        Y_UNUSED(session);
        return true;
    }

    virtual bool OnAfterRemoveObject(const typename TObjectContainer::TId& objectId, const TString& userId, NCS::TEntitySession& session) const {
        Y_UNUSED(objectId);
        Y_UNUSED(userId);
        Y_UNUSED(session);
        return true;
    }

    virtual bool ForceUpsertObject(const TObjectContainer& object, const TString& userId, NCS::TEntitySession& session, NStorage::TObjectRecordsSet<TObjectContainer>* containerExt = nullptr, bool* isUpdateExt = nullptr) const override {
        if (!object) {
            session.SetErrorInfo("incorrect user data", "upsert object into " + TObjectContainer::GetTableName(), ESessionResult::DataCorrupted);
            return false;
        }

        if (!OnBeforeUpsertObject(object, userId, session)) {
            return false;
        }
        NStorage::TTableRecord trUpdate = object.SerializeToTableRecord();
        if (!!object.GetRevisionMaybe()) {
            trUpdate.ForceSet("revision", "nextval('" + TObjectContainer::GetTableName() + "_revision_seq')");
        }
        NStorage::TTableRecord trCondition = BuildCondition(object);

        auto table = HistoryCacheDatabase->GetTable(TObjectContainer::GetTableName());

        NStorage::TObjectRecordsSet<TObjectContainer> recordsInt;
        NStorage::TObjectRecordsSet<TObjectContainer>* records = containerExt ? containerExt : &recordsInt;
        bool isUpdate = false;
        if (!trCondition.Empty()) {
            if (!table->Upsert(trUpdate, session.GetTransaction(), trCondition, &isUpdate, records)) {
                session.SetErrorInfo("upsert_object", session.GetTransaction()->GetStringReport(), ESessionResult::TransactionProblem);
                return false;
            }
            if (!HistoryManager->AddHistory(records->GetObjects(), userId, isUpdate ? EObjectHistoryAction::UpdateData : EObjectHistoryAction::Add, session)) {
                return false;
            }
        } else {
            if (!table->AddRow(trUpdate, session.GetTransaction(), "", records)) {
                session.SetErrorInfo("insert_object", session.GetTransaction()->GetStringReport(), ESessionResult::TransactionProblem);
                return false;
            }
            if (!HistoryManager->AddHistory(records->GetObjects(), userId, EObjectHistoryAction::Add, session)) {
                return false;
            }
        }
        if (records->GetObjects().size() == 1) {
            if (!OnAfterUpsertObject(records->GetObjects().front(), userId, session)) {
                return false;
            }
        }
        if (isUpdateExt) {
            *isUpdateExt = isUpdate;
        }

        return true;
    }

    bool AddObject(const TObjectContainer& object, const TString& userId, NCS::TEntitySession& session, TObjectContainer* insertedObject = nullptr) const {
        NStorage::TObjectRecordsSet<TObjectContainer> resultObjects;
        if (!AddObjects({ object }, userId, session, &resultObjects)) {
            TFLEventLog::Error("cannot add objects");
            return false;
        }
        if (resultObjects.size() != 1) {
            TFLEventLog::Error("incorrect inserted objects")("expected_count", 1)("real_count", resultObjects.size());
            return false;
        }
        if (insertedObject) {
            *insertedObject = resultObjects.back();
        }
        return true;
    }

    virtual bool AddObjects(const TVector<TObjectContainer>& objects, const TString& userId, NCS::TEntitySession& session, NStorage::TObjectRecordsSet<TObjectContainer>* containerExt = nullptr) const override {
        TRecordsSet recordsRequest;

        for (auto&& i : objects) {
            recordsRequest.AddRow(i.SerializeToTableRecord());
        }
        auto table = HistoryCacheDatabase->GetTable(TObjectContainer::GetTableName());
        NStorage::TObjectRecordsSet<TObjectContainer> recordsInt;
        NStorage::TObjectRecordsSet<TObjectContainer>* records = containerExt ? containerExt : &recordsInt;
        auto result = table->AddRows(recordsRequest, session.GetTransaction(), "", records);
        if (!result->IsSucceed() || records->size() != objects.size()) {
            session.SetErrorInfo("insert_object", session.GetTransaction()->GetStringReport(), ESessionResult::TransactionProblem);
            return false;
        }
        if (!HistoryManager->AddHistory(records->GetObjects(), userId, EObjectHistoryAction::Add, session)) {
            return false;
        }

        return true;
    }
};

class IDBMetaSnapshot {
public:
    using TPtr = TAtomicSharedPtr<IDBMetaSnapshot>;
    virtual ~IDBMetaSnapshot() = default;
};

template <class TObjectContainer>
class TDBMetaSnapshotSimple: public IDBMetaSnapshot {
private:
    TMap<typename TObjectContainer::TId, TAtomicSharedPtr<TObjectContainer>> Objects;
public:
    TDBMetaSnapshotSimple(const TVector<TObjectContainer>& objects) {
        TMap<typename TObjectContainer::TId, TAtomicSharedPtr<TObjectContainer>> objectsMap;
        for (auto&& i : objects) {
            typename TObjectContainer::TId id = i.GetInternalId();
            objectsMap.emplace(id, new TObjectContainer(i));
        }
        std::swap(Objects, objectsMap);
    }

    const typename TMap<typename TObjectContainer::TId, TAtomicSharedPtr<TObjectContainer>>::const_iterator begin() const {
        return Objects.begin();
    }

    const typename TMap<typename TObjectContainer::TId, TAtomicSharedPtr<TObjectContainer>>::const_iterator end() const {
        return Objects.end();
    }

    virtual TAtomicSharedPtr<TObjectContainer> GetObjectById(const typename TObjectContainer::TId& id) const {
        auto it = Objects.find(id);
        if (it == Objects.end()) {
            return nullptr;
        } else {
            return it->second;
        }
    }
};

template <class TObjectContainer, class TExternalSnapshotClass = TDBMetaSnapshotSimple<TObjectContainer>>
class TDBEntitiesSnapshotsConstructor {
public:
    using TSnapshotClass = TExternalSnapshotClass;
private:
    TAtomicSharedPtr<TSnapshotClass> SnapshotPtr;
protected:
    mutable TRWMutex SnapshotMutex;
public:
    virtual bool BuildSnapshots(const TVector<TObjectContainer>& objects) {
        auto snapshotPtr = MakeAtomicShared<TSnapshotClass>(objects);
        TWriteGuard wg(SnapshotMutex);
        std::swap(snapshotPtr, SnapshotPtr);
        return true;
    }

    TAtomicSharedPtr<TSnapshotClass> GetMainSnapshotPtr() const {
        TReadGuard rg(SnapshotMutex);
        CHECK_WITH_LOG(!!SnapshotPtr);
        return SnapshotPtr;
    }
};

template <class TObjectContainer, class TExternalBaseSnapshots = TDBEntitiesSnapshotsConstructor<TObjectContainer>>
class TDBMetaEntitiesManager: public TDBEntitiesManager<TObjectContainer> {
private:
    using TBase = TDBEntitiesManager<TObjectContainer>;
    using TBaseSnapshots = TExternalBaseSnapshots;
    TExternalBaseSnapshots Snapshots;
    TThreadPool SnapshotRefreshPool;
    class TSnapshotRefreshAgent: public IObjectInQueue {
    private:
        TDBMetaEntitiesManager* Owner;
    public:
        TSnapshotRefreshAgent(TDBMetaEntitiesManager* owner)
            : Owner(owner) {

        }

        virtual void Process(void* /*threadSpecificResource*/) override {
            while (Owner->IsActive()) {
                Owner->BuildSnapshots();
                Sleep(TDuration::Seconds(1));
            }
        }
    };
    TAtomic SnapshotsInitializedFlag = 0;
protected:
    virtual bool DoRebuildCacheUnsafe() const override {
        if (!TBase::DoRebuildCacheUnsafe()) {
            return false;
        }
        return true;
    }

    bool BuildSnapshots() {
        TVector<TObjectContainer> objects;
        if (!TBase::GetAllObjects(objects)) {
            return false;
        }
        return Snapshots.BuildSnapshots(objects);
    }

    virtual bool DoStart() override {
        if (!TBase::DoStart()) {
            return false;
        }
        SnapshotRefreshPool.Start(1);
        SnapshotRefreshPool.SafeAddAndOwn(MakeHolder<TSnapshotRefreshAgent>(this));
        BuildSnapshots();
        AtomicSet(SnapshotsInitializedFlag, 1);
        return true;
    }

    virtual bool DoStop() override {
        AtomicSet(SnapshotsInitializedFlag, 0);
        SnapshotRefreshPool.Stop();
        if (!TBase::DoStop()) {
            return false;
        }
        return true;
    }
public:
    using TBase::TBase;
    bool SnapshotsInitialized() const {
        return AtomicGet(SnapshotsInitializedFlag) == 1;
    }

    const TExternalBaseSnapshots& GetSnapshots() const {
        return Snapshots;
    }
};

