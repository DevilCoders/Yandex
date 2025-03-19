#pragma once
#include "abstract.h"
#include "config.h"
#include "filter.h"
#include "object.h"
#include "special_tag.h"
#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/library/storage/selection/iterator.h>

class TTagDescriptionsManager: public TDBMetaEntitiesManager<TDBTagDescription>, public ITagDescriptions {
private:
    using TBase = TDBMetaEntitiesManager<TDBTagDescription>;
    const IBaseServer& Server;

    virtual bool ParsingStrictValidation() const override {
        return ICSOperator::GetServer().GetSettings().GetValueDef("validations.tags_descriptions_manager.parsing", true);
    }

public:
    TTagDescriptionsManager(THolder<IHistoryContext>&& context, const TTagDescriptionsManagerConfig& config, const IBaseServer& server);

    ~TTagDescriptionsManager();

    virtual TDBTagDescription GetTagDescription(const TString& name) const override {
        auto td = TBase::GetCustomObject(name);
        if (td) {
            return *td;
        }
        return TDBTagDescription();
    }

    template <class TTagDescription>
    TAtomicSharedPtr<TTagDescription> GetTagDescription(const TString& name) const {
        auto td = TBase::GetCustomObject(name);
        if (!td) {
            return nullptr;
        }
        return td->GetPtrAs<TTagDescription>();
    }

    template <class TTag>
    TVector<TDBTagDescription> GetAllTagDescriptionsDerivedByClass() const {
        TVector<TDBTagDescription> result;
        TVector<TDBTagDescription> tagDescriptions;
        GetAllObjects(tagDescriptions);
        for (auto&& i : tagDescriptions) {
            auto td = i.GetPtrAs<typename TTag::TTagDescription>();
            if (!td) {
                continue;
            }
            result.emplace_back(i);
        }
        return result;
    }

    template <class TTag>
    TSet<TString> GetAllTagNamesDerivedByClass() const {
        TSet<TString> result;
        TVector<TDBTagDescription> resultTags = GetAllTagDescriptionsDerivedByClass<TTag>();
        for (auto&& i : resultTags) {
            result.emplace(i.GetName());
        }
        return result;
    }

    TSet<TString> GetAvailableNames(const TString& className = "") const {
        TVector<TDBTagDescription> tdResult = GetCachedObjectsVector();
        TSet<TString> result;
        for (auto&& i : tdResult) {
            if (!className || className == i.GetClassName()) {
                result.emplace(i.GetName());
            }
        }
        return result;
    }

    template <class T>
    TVector<TDBTagDescription> GetTagDescriptions() const {
        TVector<TDBTagDescription> result = GetCachedObjectsVector();
        const auto pred = [](const TDBTagDescription& td) {
            return !td.Is<T>();
        };
        result.erase(std::remove_if(result.begin(), result.end(), pred), result.end());
        return result;
    }

    bool UpsertTagDescriptions(const TVector<TDBTagDescription>& tDescriptions, const TString& userId, NCS::TEntitySession& session) const;
    bool RemoveTagDescriptions(const TSet<TString>& tagDescriptionsIds, const TString& userId, const bool removeDeprecated, NCS::TEntitySession& session) const;
};

template <class TDBObjectTag>
class TTaggedObject {
private:
    template <class T>
    using TDBTagSpecial = NCS::TDBTagSpecialImpl<T, TDBObjectTag>;

    CSA_READONLY_DEF(typename TDBObjectTag::TObjectId, ObjectId);
    CSA_DEFAULT(TTaggedObject, TVector<TDBObjectTag>, Tags);

public:
    typename TVector<TDBObjectTag>::const_iterator begin() const {
        return Tags.begin();
    }

    typename TVector<TDBObjectTag>::const_iterator end() const {
        return Tags.end();
    }

    TTaggedObject(const typename TDBObjectTag::TObjectId& objectId, const TVector<TDBObjectTag>& tags)
        : ObjectId(objectId)
        , Tags(tags)
    {
    }

    TTaggedObject(const typename TDBObjectTag::TObjectId& objectId)
        : ObjectId(objectId)
    {
    }

    TTaggedObject(const typename TDBObjectTag::TObjectId& objectId, TVector<TDBObjectTag>&& tags)
        : ObjectId(objectId)
        , Tags(std::move(tags))
    {
    }

    template <class T>
    TVector<TDBTagSpecial<T>> GetTags() const {
        TVector<TDBTagSpecial<T>> result;
        for (auto&& i : Tags) {
            if (i.template Is<T>()) {
                result.emplace_back(i);
            }
        }
        return result;
    }

    TVector<TDBObjectTag> GetTagsById(const TSet<typename TDBObjectTag::TTagId>& tagIds) const {
        TVector<TDBObjectTag> result;
        for (auto&& i : Tags) {
            if (tagIds.contains(i.GetTagId())) {
                result.emplace_back(i);
            }
        }
        return result;
    }

    template <class T>
    TVector<TDBTagSpecial<T>> GetTags(const TString& tagName) const {
        TVector<TDBTagSpecial<T>> result;
        for (auto&& i : Tags) {
            if (i.template Is<T>()) {
                if (i->GetName() != tagName) {
                    continue;
                }
                result.emplace_back(i);
            }
        }
        return result;
    }

    template <class T>
    TAtomicSharedPtr<T> GetFirstTag() const {
        for (auto&& i : Tags) {
            if (i.template Is<T>()) {
                return i.template GetPtrAs<T>();
            }
        }
        return nullptr;
    }

    template <class T>
    TVector<TAtomicSharedPtr<T>> GetTagPtrs(const TSet<TString>& tagNames) const {
        TVector<TAtomicSharedPtr<T>> result;
        for (auto&& i : Tags) {
            if (i.template Is<T>()) {
                if (!tagNames.contains(i->GetName())) {
                    continue;
                }
                result.emplace_back(i.template GetPtrAs<T>());
            }
        }
        return result;
    }

    template <class T>
    TDBTagSpecial<T> GetOnceTag() const {
        TDBTagSpecial<T> result;
        for (auto&& i : Tags) {
            if (i.template Is<T>()) {
                if (!result) {
                    result = i;
                } else {
                    return TDBTagSpecial<T>();
                }
            }
        }
        return result;
    }

    template <class T>
    TDBTagSpecial<T> GetOnceTag(const TString& tagName) const {
        TDBTagSpecial<T> result;
        for (auto&& i : Tags) {
            if (i.template Is<T>()) {
                if (tagName != i->GetName()) {
                    continue;
                }
                if (!result) {
                    result = i;
                } else {
                    return TDBTagSpecial<T>();
                }
            }
        }
        return result;
    }

    TVector<TDBObjectTag> GetTags() {
        return Tags;
    }
};

template <class TDBObjectTag>
class TTagsHistoryManager: public TDBEntitiesHistoryManager<TDBObjectTag> {
    using TBase = TDBEntitiesHistoryManager<TDBObjectTag>;
    using TIdRef = typename TBase::TIdRef;

protected:
    class TObjectIndex: public IEventsIndex<TObjectEvent<TDBObjectTag>, typename TDBObjectTag::TObjectId> {
    private:
        using TBase = IEventsIndex<TObjectEvent<TDBObjectTag>, typename TDBObjectTag::TObjectId>;

    protected:
        virtual TMaybe<typename TDBObjectTag::TObjectId> ExtractKey(TAtomicSharedPtr<TObjectEvent<TDBObjectTag>> ev) const override {
            return ev->GetObjectId();
        }

    public:
        using TBase::TBase;
    };

    THolder<TObjectIndex> ObjectIndex;

public:
    using TBase::TBase;

    const TObjectIndex& GetIndexByObjectId() const {
        return *ObjectIndex;
    }

    TTagsHistoryManager(const IHistoryContext& context, const THistoryConfig& config)
        : TBase(context, config)
        , ObjectIndex(MakeHolder<TObjectIndex>(this, "object_id"))
    {
    }
};

template <class TTagObject>
class IDirectTagsManager {
public:
    virtual bool SetTagsPerformer(const TSet<typename TTagObject::TTagId>& tagIds, const TString& userId, NCS::TEntitySession& session, TVector<TTagObject>* dbTagsResult = nullptr) const = 0;
    virtual bool DropTagsPerformer(const TSet<typename TTagObject::TTagId>& tagIds, const TString& userId, NCS::TEntitySession& session, TVector<TTagObject>* dbTagsResult = nullptr) const = 0;
    virtual bool SmartUpsert(const TTagObject& container, const TDBTagDescription& td, const TString& userId, NCS::TEntitySession& session, bool* isUpdate = nullptr, TTagObject* tagResult = nullptr) const = 0;
    virtual bool RemoveTags(const TSet<typename TTagObject::TTagId> & tagIds, const TString & userId, NCS::TEntitySession & session, TVector<TTagObject> * dbTagsResult = nullptr) const = 0;
    virtual bool RestoreSortedObjectsById(const TSet<typename TTagObject::TObjectId>& objectIds, TVector<TTaggedObject<TTagObject>>& taggedObjects, NCS::TEntitySession& session) const = 0;
    virtual bool AddObjectTags(const TVector<ITag::TPtr>& tags, const typename TTagObject::TObjectId& objectId, const TString& userId, NCS::TEntitySession& session, TVector<TTagObject>* dbTagsResult = nullptr) const = 0;

    virtual bool RestoreObjectsByTagIds(const TSet<typename TTagObject::TTagId>& tagIds, TVector<TTaggedObject<TTagObject>>& taggedObjects, const bool allObjectTags, NCS::TEntitySession& session) const = 0;
    virtual NCS::TEntitySession BuildNativeSession(const bool readOnly = false, const bool repeatableRead = false) const = 0;

    static NCS::NSelection::TReader<TTagObject, NCS::NTags::TSelection> BuildSequentialReader() {
        return NCS::NSelection::TReader<TTagObject, NCS::NTags::TSelection>(TTagObject::GetTableName());
    }
    static NCS::NSelection::TReader<TTagObject, NCS::NTags::TObjectsSelection> BuildObjectsSequentialReader() {
        return NCS::NSelection::TReader<TTagObject, NCS::NTags::TObjectsSelection>(TTagObject::GetTableName());
    }
    virtual ~IDirectTagsManager() = default;

    bool RestoreSortedObjectsById(const TSet<typename TTagObject::TObjectId>& objectIds, TMap<typename TTagObject::TObjectId, TTaggedObject<TTagObject>>& result, NCS::TEntitySession& session) const {
        TVector<TTaggedObject<TTagObject>> objects;
        if (!RestoreSortedObjectsById(objectIds, objects, session)) {
            return false;
        }
        TMap<typename TTagObject::TObjectId, TTaggedObject<TTagObject>> resultLocal;
        for (auto&& i : objects) {
            const typename TTagObject::TObjectId objectId = i.GetObjectId();
            resultLocal.emplace(objectId, std::move(i));
        }
        std::swap(resultLocal, result);
        return true;
    }

    TVector<TTaggedObject<TTagObject>> BuildTaggedObjectsSorted(TVector<TTagObject>&& dbTags) const {
        TVector<TTaggedObject<TTagObject>> result;
        const auto pred = [](const TTagObject& l, const TTagObject& r) {
            return l.GetObjectId() < r.GetObjectId();
        };
        std::sort(dbTags.begin(), dbTags.end(), pred);
        for (auto&& i : dbTags) {
            if (result.empty() || result.back().GetObjectId() != i.GetObjectId()) {
                result.emplace_back(TTaggedObject<TTagObject>(i.GetObjectId()));
            }
            result.back().MutableTags().emplace_back(std::move(i));
        }
        return result;
    }

};

template <class TBaseClass>
class TDBDirectTagsManager: public TBaseClass, public IDirectTagsManager<typename TBaseClass::TEntity> {
public:
    using TDBObjectTag = typename TBaseClass::TEntity;
    using TTaggedObject = TTaggedObject<TDBObjectTag>;
private:
    using TBaseInterface = IDirectTagsManager<typename TBaseClass::TEntity>;
    using TBase = TBaseClass;
    const IBaseServer& Server;

protected:
    const IBaseServer& GetBaseServer() const {
        return Server;
    }

public:
    using TBase::TBase;

    virtual NCS::TEntitySession BuildNativeSession(const bool readOnly = false, const bool repeatableRead = false) const override {
        return TBase::BuildNativeSession(readOnly, repeatableRead);
    }

    template <class T, class TMaybePolicy>
    bool PatchTag(const TString& objectId, const TMaybe<T, TMaybePolicy>& tag, const TString& userId, NCS::TEntitySession& session) const {
        TTaggedObject tObject;
        if (!RestoreTaggedObject(objectId, tObject, session)) {
            return false;
        }
        auto tags = tObject.template GetTags<T>();
        if (!tag) {
            if (tags.size() == 1) {
                if (!RemoveTags({tags[0].GetDBTag().GetTagId()}, userId, session)) {
                    return false;
                }
            }
        } else {
            bool found = false;
            for (auto&& i : tags) {
                if (i->GetName() == tag->GetName() && i->IsEqual(*tag)) {
                    found = true;
                    break;
                }
            }
            if (found) {
                return true;
            }
            ITag::TPtr tagPtr = MakeAtomicShared<T>(*tag);
            if (tags.size() == 1) {
                if (!UpdateTag(tagPtr, tags[0].GetDBTag().GetTagId(), userId, session)) {
                    return false;
                }
            } else {
                if (!AddObjectTags({tagPtr}, objectId, userId, session, nullptr)) {
                    return false;
                }
            }
        }
        return true;
    }

    bool SmartUpsert(const TDBObjectTag& container, const TDBTagDescription& td, const TString& userId, NCS::TEntitySession* session = nullptr) const {
        if (session) {
            return SmartUpsert(container, td, userId, *session);
        } else {
            auto sessionLocal = TBase::BuildNativeSession(false);
            return SmartUpsert(container, td, userId, sessionLocal) && sessionLocal.Commit();
        }
    }

    bool SmartUpsert(const TDBObjectTag& container, const TString& userId, NCS::TEntitySession& session, bool* isUpdate = nullptr, TDBObjectTag* tagResult = nullptr) const {
        if (!container) {
            session.Error("incorrect tag container");
            return false;
        }
        auto td = Server.GetTagDescriptionsManager().GetCustomObject(container->GetName());
        if (!td) {
            session.Error("cannot fetch tag description");
            return false;
        }
        return SmartUpsert(container, *td, userId, session, isUpdate, tagResult);
    }

    virtual bool SmartUpsert(const TDBObjectTag& container, const TDBTagDescription& td, const TString& userId, NCS::TEntitySession& session, bool* isUpdate = nullptr, TDBObjectTag* tagResult = nullptr) const override {
        auto gLogging = TFLRecords::StartContext().Method("SmartUpsert");
        if (!container) {
            session.Error("incorrect tag container");
            return false;
        }
        if (!td || td.GetName() != container->GetName()) {
            session.Error("incorrect tag description");
            return false;
        } else {
            gLogging("tag_name", td.GetName());
        }
        if (container.GetTagId()) {
            gLogging("tag_id", container.GetTagId());
        }
        if (container.GetObjectId()) {
            gLogging("object_id", container.GetObjectId());
        }
        if (!container) {
            session.Error("empty tag container");
            return false;
        }
        if (!container.GetPtr()->OnSmartUpsert(container.GetAddress(), Server, td, session)) {
            TFLEventLog::Error("cannot patch tag with tag description");
            return false;
        }
        if (!!container.GetTagId()) {
            if (!UpdateTags<TDBObjectTag>({ container }, userId, session)) {
                TFLEventLog::Error("cannot update tag");
                return false;
            }
            if (isUpdate) {
                *isUpdate = true;
            }
            if (tagResult) {
                *tagResult = container;
            }
        } else if (!!container.GetObjectId()) {
            if (td->GetUniquePolicy() == ITagDescription::EUniquePolicy::NonUnique) {
                TVector<TDBObjectTag> addedTags;
                if (!AddObjectTags({container.GetPtr()}, container.GetObjectId(), userId, session, &addedTags)) {
                    TFLEventLog::Error("cannot add tag");
                    return false;
                }
                if (addedTags.empty()) {
                    session.Error("for add tags with non-exists condition: not tags added");
                    return false;
                }
                if (isUpdate) {
                    *isUpdate = false;
                }
                if (tagResult) {
                    *tagResult = addedTags.front();
                }
            } else if (td->GetUniquePolicy() == ITagDescription::EUniquePolicy::AddIfNotExists) {
                TVector<TDBObjectTag> addedTags;
                if (!AddTagIfNotExistsByName(container.GetPtr(), container.GetObjectId(), userId, session, &addedTags)) {
                    TFLEventLog::Error("cannot add tag with non-exists condition");
                    return false;
                }
                if (addedTags.empty()) {
                    session.Error("for add tags with non-exists condition: not tags added");
                    return false;
                }
                if (isUpdate) {
                    *isUpdate = false;
                }
                if (tagResult) {
                    *tagResult = addedTags.front();
                }
            } else if (td->GetUniquePolicy() == ITagDescription::EUniquePolicy::UpsertIfExists) {
                TVector<TDBObjectTag> addedTags;
                if (!UpsertTagByName(container.GetPtr(), container.GetObjectId(), userId, session, &addedTags, isUpdate)) {
                    TFLEventLog::Error("cannot upsert tags");
                    return false;
                }
                if (addedTags.empty()) {
                    session.Error("for add tags with non-exists condition: not tags upserted");
                    return false;
                }
                if (tagResult) {
                    *tagResult = addedTags.front();
                }
            } else {
                session.Error("incorrect tag unique policy");
                return false;
            }
        } else {
            session.Error("tag withno object_id/tag_id");
            return false;
        }
        return true;
    }

    TVector<TTaggedObject> BuildTaggedObjects(TVector<TDBObjectTag>&& dbTags) const {
        return TBaseInterface::BuildTaggedObjectsSorted(std::move(dbTags));
    }

    bool RestoreTaggedObjects(TVector<TTaggedObject>& result) const {
        TVector<TDBObjectTag> tags;
        if (!TBase::RestoreAllObjects(tags)) {
            ALERT_LOG << "Failed to RestoreAllObjects from TBase" << Endl;
            return false;
        }
        result = BuildTaggedObjects(std::move(tags));
        return true;
    }

    bool RestoreTaggedObjects(TVector<TTaggedObject>& result, NCS::TEntitySession& session) const {
        TVector<TDBObjectTag> tags;
        if (!TBase::RestoreAllObjects(tags, session)) {
            ALERT_LOG << "Failed to RestoreAllObjects from TBase" << Endl;
            return false;
        }
        result = BuildTaggedObjects(std::move(tags));
        return true;
    }

    bool RestoreByClassName(const TSet<TString>& classNames, TVector<TDBObjectTag>& dbTags, NCS::TEntitySession& session) const {
        if (classNames.empty()) {
            return true;
        }
        return TBase::RestoreObjectsBySRCondition(dbTags, TSRCondition().Init<TSRBinary>("class_name", classNames), session);
    }

    bool RestoreByNames(const TSet<TString>& tagNames, TVector<TDBObjectTag>& dbTags, NCS::TEntitySession& session) const {
        if (tagNames.empty()) {
            return true;
        }
        return TBase::RestoreObjectsBySRCondition(dbTags, TSRCondition().Init<TSRBinary>("tag_name", tagNames), session);
    }

    bool RestoreTaggedObject(const typename TDBObjectTag::TObjectId& objectId, TTaggedObject& result, NCS::TEntitySession& session) const {
        TVector<TTaggedObject> taggedObjects;
        if (!RestoreSortedObjectsById({ objectId }, taggedObjects, session)) {
            return false;
        }
        TTaggedObject resultLocal(objectId);
        if (taggedObjects.size()) {
            resultLocal = taggedObjects.front();
        }
        std::swap(result, resultLocal);
        return true;
    }

    TSRQuery BuildRequestRestoreByObjectIds(const TSet<typename TDBObjectTag::TObjectId>& objectIds) const {
        if (objectIds.empty()) {
            return nullptr;
        }
        THolder<TSRSelect> result(MakeHolder<TSRSelect>(TDBObjectTag::GetTableName()));
        result->RetCondition<TSRBinary>(ESRBinary::In).InitLeft<TSRFieldName>("object_id").RetRight<TSRMulti>().FillValues(objectIds);
        return result.Release();
    }

    virtual bool RestoreSortedObjectsById(const TSet<typename TDBObjectTag::TObjectId>& objectIds, TVector<TTaggedObject>& taggedObjects, NCS::TEntitySession& session) const override {
        if (objectIds.empty()) {
            return true;
        }
        TVector<TDBObjectTag> dbTags;
        if (!TBase::RestoreObjectsBySRCondition(dbTags, TSRCondition().Init<TSRBinary>("object_id", objectIds), session)) {
            return false;
        }
        taggedObjects = BuildTaggedObjects(std::move(dbTags));
        return true;
    }

    virtual bool RestoreObjectsByTagIds(const TSet<typename TDBObjectTag::TTagId>& tagIds, TVector<TTaggedObject>& taggedObjects, const bool allObjectTags, NCS::TEntitySession& session) const override {
        if (tagIds.empty()) {
            return true;
        }
        TVector<TDBObjectTag> dbTags;

        TSRCondition srCondition;
        {
            TSRCondition srBaseCondition;
            srBaseCondition.Init<TSRBinary>("tag_id", tagIds);
            if (allObjectTags) {
                auto& rSelect = srCondition.Ret<TSRBinary>(ESRBinary::In).InitLeft<TSRFieldName>("object_id").RetRight<TSRSelect>(TDBObjectTag::GetTableName());
                rSelect.template InitFields<TSRFieldName>("object_id").SetCondition(srBaseCondition).Distinct(true);
            } else {
                srCondition = srBaseCondition;
            }
        }

        if (!TBase::RestoreObjectsBySRCondition(dbTags, srCondition, session)) {
            return false;
        }
        taggedObjects = BuildTaggedObjects(std::move(dbTags));
        return true;
    }

    bool RestoreObjectsByTagNames(const TSet<TString>& tagNames, TVector<TTaggedObject>& taggedObjects, const bool allObjectTags, NCS::TEntitySession& session) const {
        if (tagNames.empty()) {
            return true;
        }
        TVector<TDBObjectTag> dbTags;
        TSRCondition srCondition;
        if (!tagNames.contains("*")) {
            TSRCondition srBaseCondition;
            srBaseCondition.Init<TSRBinary>("tag_name", tagNames);
            if (allObjectTags) {
                auto& rSelect = srCondition.Ret<TSRBinary>(ESRBinary::In).InitLeft<TSRFieldName>("object_id").RetRight<TSRSelect>(TDBObjectTag::GetTableName());
                rSelect.template InitFields<TSRFieldName>("object_id").SetCondition(srBaseCondition).Distinct(true);
            } else {
                srCondition = srBaseCondition;
            }
        }
        if (!TBase::RestoreObjectsBySRCondition(dbTags, srCondition, session)) {
            return false;
        }
        taggedObjects = BuildTaggedObjects(std::move(dbTags));
        return true;
    }

    bool RestoreObjectsInPerforming(const TString& userId, TVector<TTaggedObject>& taggedObjects, const bool allObjectTags, NCS::TEntitySession& session) const {
        if (!userId) {
            return true;
        }
        TSRCondition srBaseCondition;
        srBaseCondition.Init<TSRBinary>("performer_id", userId);

        TSRCondition srCondition;
        if (allObjectTags) {
            auto& rSelect = srCondition.Ret<TSRBinary>(ESRBinary::In).InitLeft<TSRFieldName>("object_id").RetRight<TSRSelect>(TDBObjectTag::GetTableName());
            rSelect.template InitFields<TSRFieldName>("object_id").SetCondition(srBaseCondition).Distinct(true);
        } else {
            srCondition = srBaseCondition;
        }
        TVector<TDBObjectTag> dbTags;
        if (!TBase::RestoreObjectsBySRCondition(dbTags, srCondition, session)) {
            return false;
        }
        taggedObjects = BuildTaggedObjects(std::move(dbTags));
        return true;
    }

    TDBDirectTagsManager(const IHistoryContext& context, const TTagsManagerConfig& config, const IBaseServer& server)
        : TBase(context, config.GetHistoryConfig())
        , Server(server)
    {
    }

    TDBDirectTagsManager(NStorage::IDatabase::TPtr db, const THistoryConfig& hConfig, const IBaseServer& server)
        : TBase(db, hConfig)
        , Server(server)
    {
    }

    virtual bool AddObjectTags(const TVector<ITag::TPtr>& tags, const typename TDBObjectTag::TObjectId& objectId, const TString& userId, NCS::TEntitySession& session, TVector<TDBObjectTag>* dbTagsResult = nullptr) const override {
        TVector<TDBObjectTag> commonTags;
        TRecordsSet records;
        for (auto&& i : tags) {
            if (!i) {
                TFLEventLog::Error("empty tag for add");
                return false;
            }
            TDBObjectTag dbTag(i);
            dbTag.SetObjectId(objectId);
            commonTags.emplace_back(std::move(dbTag));
        }
        return AddTagsImpl(commonTags, userId, session, nullptr, dbTagsResult);
    }

    bool AddTags(const TVector<TDBObjectTag>& tags, const TString& userId, NCS::TEntitySession& session, TVector<TDBObjectTag>* dbTagsResult = nullptr) const {
        return AddTagsImpl(tags, userId, session, nullptr, dbTagsResult);
    }

    bool AddTagIfNotExistsByName(const ITag::TPtr& tag, const typename TDBObjectTag::TObjectId& objectId, const TString& userId, NCS::TEntitySession& session, TVector<TDBObjectTag>* dbTagsResult = nullptr) const {
        if (!tag) {
            session.Error("incorrect tag for AddObjectTagIfNotExistsByname method");
            return false;
        }
        NStorage::TTableRecord condition;
        condition.Set("tag_name", tag->GetName());
        condition.Set("object_id", objectId);
        TDBObjectTag dbTag(tag);
        dbTag.SetObjectId(objectId);
        TRecordsSet rs;
        rs.AddRow(dbTag.SerializeToTableRecord());

        return AddTagsImpl({dbTag}, userId, session, &condition, dbTagsResult);
    }

    bool UpsertTagByName(const ITag::TPtr& tag, const typename TDBObjectTag::TObjectId& objectId, const TString& userId, NCS::TEntitySession& session, TVector<TDBObjectTag>* dbTagsResult = nullptr, bool* isUpdateExt = nullptr) const {
        if (!tag) {
            session.Error("incorrect tag for AddObjectTagIfNotExistsByname method");
            return false;
        }
        NStorage::TTableRecord condition;
        condition.Set("tag_name", tag->GetName());
        condition.Set("object_id", objectId);
        TDBObjectTag dbTag(tag);
        dbTag.SetObjectId(objectId);

        bool isUpdate = false;
        TVector<TDBObjectTag> updates;
        if (!TBase::UpsertRecord(dbTag.SerializeToTableRecord(), condition, userId, session, &updates, &isUpdate)) {
            return false;
        }

        if (!isUpdate) {
            for (auto&& tagUpdates : updates) {
                if (!tagUpdates->OnAfterAdd(tagUpdates.GetAddress(), Server, userId, session)) {
                    return false;
                }
            }
        }
        if (isUpdateExt) {
            *isUpdateExt = isUpdate;
        }

        if (dbTagsResult) {
            *dbTagsResult = updates;
        }
        return true;
    }

    bool RemoveByObjectIds(const TSet<typename TDBObjectTag::TObjectId>& objectIds, const TString& userId, NCS::TEntitySession& session) const {
        if (objectIds.empty()) {
            return true;
        }
        TVector<TDBObjectTag> tags;
        if (!TBase::RestoreObjectsBySRCondition(tags, TSRCondition().Init<TSRBinary>("object_id", objectIds), session)) {
            return false;
        }
        for (auto&& i : tags) {
            if (!i->OnBeforeRemove(i.GetAddress(), Server, userId, session)) {
                return false;
            }
        }
        TSRCondition condition;
        condition.Init<TSRBinary>("object_id", objectIds);
        return TBase::RemoveRecords(condition, userId, session);
    }

    virtual bool RemoveTags(const TSet<typename TDBObjectTag::TTagId>& tagIds, const TString& userId, NCS::TEntitySession& session, TVector<TDBObjectTag>* dbTagsResult = nullptr) const override {
        if (tagIds.empty()) {
            return true;
        }
        TVector<TDBObjectTag> tags;
        if (!TBase::RestoreObjectsBySRCondition(tags, TSRCondition().Init<TSRBinary>("tag_id", tagIds), session)) {
            return false;
        }
        for (auto&& i : tags) {
            if (!i->OnBeforeRemove(i.GetAddress(), Server, userId, session)) {
                return false;
            }
        }
        if (dbTagsResult) {
            *dbTagsResult = std::move(tags);
        }
        TSRCondition condition;
        condition.Init<TSRBinary>("tag_id", tagIds);
        return TBase::RemoveRecords(condition, userId, session);
    }

    virtual bool SetTagsPerformer(const TSet<typename TDBObjectTag::TTagId>& tagIds, const TString& userId, NCS::TEntitySession& session, TVector<TDBObjectTag>* dbTagsResult = nullptr) const override {
        if (tagIds.empty()) {
            return true;
        }
        TSRCondition srUpdate;
        srUpdate.RetObject<TSRBinary>("performer_id", userId);
        TSRCondition srCondition;
        auto& srMulti = srCondition.RetObject<TSRMulti>();
        srMulti.InitNode<TSRBinary>("tag_id", tagIds);
        srMulti.InitNode<TSRNullChecker>("performer_id");
        return TBase::UpdateRecord(srUpdate, srCondition, userId, session, dbTagsResult, -1);
    }

    virtual bool DropTagsPerformer(const TSet<typename TDBObjectTag::TTagId>& tagIds, const TString& userId, NCS::TEntitySession& session, TVector<TDBObjectTag>* dbTagsResult = nullptr) const override {
        if (tagIds.empty()) {
            return true;
        }
        TSRCondition srUpdate;
        srUpdate.RetObject<TSRNullInit>("performer_id");
        TSRCondition srCondition;
        auto& srMulti = srCondition.RetObject<TSRMulti>();
        srMulti.InitNode<TSRBinary>("tag_id", tagIds);
        srMulti.InitNode<TSRBinary>("performer_id", userId);
        return TBase::UpdateRecord(srUpdate, srCondition, userId, session, dbTagsResult, -1);
    }

    template <class TDBTagExternal>
    bool UpdateTags(const TVector<TDBTagExternal>& tags, const TString& userId, NCS::TEntitySession& session) const {
        TVector<NStorage::TTableRecord> updates;
        TVector<NStorage::TTableRecord> conditions;
        for (auto&& i : tags) {
            if (!i) {
                session.Error("empty tag for add");
                return false;
            }

            NStorage::TTableRecord update = i.SerializeToTableRecord();
            update.Remove("object_id");
            NStorage::TTableRecord condition;
            if (!NCS::NStorage::TTagIdWriter<typename TDBTagExternal::TTagId>::Write(condition, "tag_id", i.GetTagId())) {
                session.Error("cannot write tag_id");
                return false;
            }

            updates.emplace_back(std::move(update));
            conditions.emplace_back(std::move(condition));
        }
        if (!TBase::UpdateRecords(updates, conditions, userId, session, nullptr)) {
            return false;
        }
        return true;
    }

    bool RenameTags(const TSet<TString>& tagIds, const TString& newTagName, const TString& userId, NCS::TEntitySession& session) const {
        if (tagIds.empty()) {
            return true;
        }
        const auto gLogging = TFLRecords::StartContext().Method("RenameTags")("table_name", TDBObjectTag::GetTableName());
        NStorage::TObjectRecordsSet<TDBObjectTag> dbObjects;
        TSRUpdate srUpdate(TDBObjectTag::GetTableName(), &dbObjects);
        srUpdate.InitCondition<TSRBinary>("tag_id", tagIds);
        srUpdate.InitUpdate<TSRBinary>("tag_name", newTagName);
        if (!session.ExecRequest(srUpdate)) {
            return false;
        }
        if (!TBase::HistoryManager->AddHistory(dbObjects.GetObjects(), userId, EObjectHistoryAction::UpdateData, session)) {
            return false;
        }
        return true;
    }

    bool UpdateTag(ITag::TPtr tag, const TString& tagId, const TString& userId, NCS::TEntitySession& session) const {
        if (!tag) {
            return true;
        }
        if (!Server.GetTagDescriptionsManager().GetCustomObject(tag->GetName())) {
            TFLEventLog::Error("no description for tag")("tag_name", tag->GetName());
            return false;
        }
        NStorage::TTableRecord update = tag->SerializeToTableRecord();
        NStorage::TTableRecord condition;
        condition.Set("tag_id", tagId);
        if (!TBase::UpdateRecord(update, condition, userId, session, nullptr)) {
            return false;
        }
        return true;
    }

    bool UpdateTag(const ITag& tag, const TString& tagId, const TString& userId, NCS::TEntitySession& session) const {
        if (!Server.GetTagDescriptionsManager().GetCustomObject(tag.GetName())) {
            TFLEventLog::Error("no description for tag")("tag_name", tag.GetName());
            return false;
        }
        NStorage::TTableRecord update = tag.SerializeToTableRecord();
        NStorage::TTableRecord condition;
        condition.Set("tag_id", tagId);
        if (!TBase::UpdateRecord(update, condition, userId, session, nullptr)) {
            return false;
        }
        return true;
    }

    TMaybe<TDBTagDescription> GetTagDescription(const TString& tagName) const {
        return Server.GetTagDescriptionsManager().GetCustomObject(tagName);
    }

private:
    bool AddTagsImpl(const TVector<TDBObjectTag>& tags, const TString& userId, NCS::TEntitySession& session, const NStorage::TTableRecord* condition = nullptr, TVector<TDBObjectTag>* dbTagsResult = nullptr) const {
        TRecordsSet records;
        for (auto&& dbTag : tags) {
            if (!Server.GetTagDescriptionsManager().GetCustomObject(dbTag->GetName())) {
                TFLEventLog::Error("no description for tag")("tag_name", dbTag->GetName());
                return false;
            }
            records.AddRow(dbTag.SerializeToTableRecord());
        }

        TVector<TDBObjectTag> newTags;
        if (!TBase::AddRecords(records, userId, session, &newTags, condition)) {
            return false;
        }

        for (auto&& tag : newTags) {
            if (!tag->OnAfterAdd(tag.GetAddress(), Server, userId, session)) {
                return false;
            }
        }

        if (dbTagsResult) {
            *dbTagsResult = newTags;
        }
        return true;
    }
};

template <class TDBObjectTag, class TBaseClass = TDBDirectTagsManager<TDBEntitiesCache<TDBObjectTag, TTagsHistoryManager<TDBObjectTag>>>>
class TTagsManager: public TBaseClass {
private:
    using TBase = TBaseClass;
    using TTaggedObject = TTaggedObject<TDBObjectTag>;

    class TIndexByObjectPolicy {
    public:
        using TKey = typename TDBObjectTag::TObjectId;
        using TObject = TDBObjectTag;
        static const typename TDBObjectTag::TObjectId& GetKey(const TObject& object) {
            return object.GetObjectId();
        }
        static const typename TDBObjectTag::TTagId& GetUniqueId(const TObject& object) {
            return object.GetTagId();
        }
    };

    using TIndexByObject = TObjectByKeyIndex<TIndexByObjectPolicy>;

    mutable TIndexByObject IndexByObject;
protected:

    virtual bool ParsingStrictValidation() const override {
        return ICSOperator::GetServer().GetSettings().GetValueDef("validations.tags_manager." + TDBObjectTag::GetTableName() + ".parsing", true);
    }

    using TBase::Objects;

    virtual bool DoRebuildCacheUnsafe() const override {
        if (!TBase::DoRebuildCacheUnsafe()) {
            return false;
        }
        IndexByObject.Initialize(Objects);
        return true;
    }

    virtual void DoAcceptHistoryEventBeforeRemoveUnsafe(const TObjectEvent<TDBObjectTag>& ev) const override {
        TBase::DoAcceptHistoryEventBeforeRemoveUnsafe(ev);
        IndexByObject.Remove(ev);
    }

    virtual void DoAcceptHistoryEventAfterChangeUnsafe(const TObjectEvent<TDBObjectTag>& ev, TDBObjectTag& object) const override {
        TBase::DoAcceptHistoryEventAfterChangeUnsafe(ev, object);
        IndexByObject.Upsert(ev);
    }
public:

    TTagsManager(const IHistoryContext& context, const TTagsManagerConfig& config, const IBaseServer& server)
        : TBase(context.GetDatabase(), config.GetHistoryConfig(), server) {
    }

    TTagsManager(NStorage::IDatabase::TPtr db, const THistoryConfig& hConfig, const IBaseServer& server)
        : TBase(db, hConfig, server) {
    }

    TVector<TTaggedObject> GetCachedTaggedObjects(const TVector<TString>& onlyIds = {}) const {
        if (onlyIds.empty()) {
            TReadGuard rg(TBase::MutexCachedObjects);
            TIndexByObject indexByObject = IndexByObject;
            rg.Release();
            TVector<TTaggedObject> result;
            for (auto&& i : indexByObject.GetIndexData()) {
                result.emplace_back(TTaggedObject(i.first, i.second));
            }
            return result;
        } else {
            TReadGuard rg(TBase::MutexCachedObjects);
            TVector<TTaggedObject> result;
            for (auto&& id : onlyIds) {
                auto objects = IndexByObject.GetObjectsByKey(id);
                result.emplace_back(id, objects);
            }
            return result;
        }
    }

    bool GetTaggedObjects(TMap<TString, TTaggedObject>& result, const TInstant reqActuality = TInstant::Zero(), const TVector<TString>& onlyIds = {}) const {
        if (!TBase::RefreshCache(reqActuality)) {
            return false;
        }
        TVector<TTaggedObject> resultLocal = GetCachedTaggedObjects(onlyIds);
        TMap<TString, TTaggedObject> resultLocalMap;
        for (auto&& i : resultLocal) {
            const TString objectId = i.GetObjectId();
            resultLocalMap.emplace(objectId, std::move(i));
        }
        std::swap(result, resultLocalMap);
        return true;
    }

    TTaggedObject GetTaggedObject(const TString& objectId) const {
        TReadGuard rg(TBase::MutexCachedObjects);
        auto it = IndexByObject.GetIndexData().find(objectId);
        if (it == IndexByObject.GetIndexData().end()) {
            return TTaggedObject(objectId, {});
        } else {
            return TTaggedObject(it->first, it->second);
        }
    }

    TMaybe<TVector<TTaggedObject>> GetTaggedObjects(const TInstant reqActuality = TInstant::Zero()) const {
        if (!TBase::RefreshCache(reqActuality)) {
            return Nothing();
        }
        return GetCachedTaggedObjects();
    }

};
