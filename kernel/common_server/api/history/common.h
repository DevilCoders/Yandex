#pragma once
#include <kernel/common_server/library/interfaces/container.h>
#include <kernel/common_server/library/interfaces/proto.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/library/storage/structured.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/config.h>
#include <library/cpp/yconf/conf.h>
#include <util/generic/ptr.h>
#include <util/generic/cast.h>
#include <util/memory/blob.h>

class IHistoryContext {
public:
    virtual ~IHistoryContext() {
    }

    virtual TAtomicSharedPtr<NStorage::IDatabase> GetDatabase() const = 0;
};

class THistoryContext: public IHistoryContext {
private:
    TAtomicSharedPtr<NStorage::IDatabase> Database;
public:
    THistoryContext(TAtomicSharedPtr<NStorage::IDatabase> db)
        : Database(db) {
        CHECK_WITH_LOG(!!db);
    }

    virtual TAtomicSharedPtr<NStorage::IDatabase> GetDatabase() const override {
        return Database;
    }
};

template <class T>
class TIdTypeSelector {
public:
    using TId = T;
    using TIdRef = T;
};

template <>
class TIdTypeSelector<TString> {
public:
    using TId = TString;
    using TIdRef = TStringBuf;
};

template <class TPolicy>
class TObjectByKeyIndex {
private:
    using TKey = typename TPolicy::TKey;
    using TObject = typename TPolicy::TObject;
    using TUniqueKey = typename TObject::TId;
    TMap<TKey, TVector<TObject>> IndexedObjects;
    TMap<TUniqueKey, TKey> CurrentKeyForObject;
    CSA_DEFAULT(TObjectByKeyIndex, TPolicy, Policy);
public:
    const TMap<TKey, TVector<TObject>>& GetIndexData() const {
        return IndexedObjects;
    }

    TVector<TObject> GetObjectsByKey(const TKey& key) const {
        auto it = IndexedObjects.find(key);
        if (it != IndexedObjects.end()) {
            return it->second;
        }
        return TVector<TObject>();
    }

    size_t GetObjectsCountByKey(const TKey& key) const {
        auto it = IndexedObjects.find(key);
        if (it != IndexedObjects.end()) {
            return it->second.size();
        }
        return 0;
    }

    TVector<TObject> GetObjectsByKeys(const TSet<TKey>& keys, const ui32 objectsCountLimit = Max<ui32>()) const {
        TVector<TObject> result;
        if (!objectsCountLimit) {
            return result;
        }
        for (auto&& k : keys) {
            auto it = IndexedObjects.find(k);
            if (it == IndexedObjects.end()) {
                continue;
            }
            const ui32 proposedSize = result.size() + it->second.size();
            if (proposedSize <= objectsCountLimit) {
                result.reserve(proposedSize);
                result.insert(result.end(), it->second.begin(), it->second.end());
            } else {
                result.reserve(objectsCountLimit);
                result.insert(result.end(), it->second.begin(), it->second.begin() + (objectsCountLimit - result.size()));
            }
        }
        return result;
    }

    template <class TId, class TDerivObject>
    bool Initialize(const TMap<TId, TDerivObject>& objects) {
        IndexedObjects.clear();
        for (auto&& i : objects) {
            const auto& key = Policy.GetKey(i.second);
            if (!key) {
                continue;
            }
            IndexedObjects[key].emplace_back(i.second);
            CurrentKeyForObject.emplace(Policy.GetUniqueId(i.second), key);
        }
        return true;
    }

    bool Upsert(const TObject& object) {
        const auto& newObjectKey = Policy.GetKey(object);
        if (!newObjectKey) {
            return Remove(object);
        }

        auto objectId = Policy.GetUniqueId(object);
        auto itCurrentKey = CurrentKeyForObject.find(objectId);
        if (itCurrentKey != CurrentKeyForObject.end()) {
            auto currentIndex = IndexedObjects.find(itCurrentKey->second);
            CHECK_WITH_LOG(currentIndex != IndexedObjects.end());
            if (newObjectKey != itCurrentKey->second) {
                const auto pred = [this, &objectId](const TObject& object) {
                    return Policy.GetUniqueId(object) == objectId;
                };
                currentIndex->second.erase(std::remove_if(currentIndex->second.begin(), currentIndex->second.end(), pred), currentIndex->second.end());
                if (currentIndex->second.empty()) {
                    IndexedObjects.erase(currentIndex);
                }
            } else {
                for (auto&& i : currentIndex->second) {
                    if (Policy.GetUniqueId(i) == objectId) {
                        i = object;
                        return true;
                    }
                }
            }
            itCurrentKey->second = newObjectKey;
        } else {
            CurrentKeyForObject.emplace(objectId, newObjectKey);
        }
        auto it = IndexedObjects.find(newObjectKey);
        if (it == IndexedObjects.end()) {
            IndexedObjects.emplace(newObjectKey, TVector<TObject>({ object }));
        } else {
            it->second.emplace_back(object);
        }
        return true;
    }
    bool Remove(const TObject& object) {
        return Remove(Policy.GetUniqueId(object));
    }
    bool Remove(const typename TObject::TId& objectId) {
        auto itCurrentKey = CurrentKeyForObject.find(objectId);
        if (itCurrentKey == CurrentKeyForObject.end()) {
            return true;
        }
        auto it = IndexedObjects.find(itCurrentKey->second);
        CHECK_WITH_LOG(it != IndexedObjects.end());
        const auto pred = [this, &objectId](const TObject& object) {
            return Policy.GetUniqueId(object) == objectId;
        };
        it->second.erase(std::remove_if(it->second.begin(), it->second.end(), pred), it->second.end());
        if (it->second.empty()) {
            IndexedObjects.erase(it);
        }
        CurrentKeyForObject.erase(itCurrentKey);
        return true;
    }
};

template <class TPolicy>
class TObjectsCountByKeyIndex {
private:
    using TKey = typename TPolicy::TKey;
    using TObject = typename TPolicy::TObject;
    using TUniqueKey = TString;
    TMap<TKey, i32> IndexedObjects;
    TMap<TUniqueKey, TKey> CurrentKeyForObject;
    CSA_DEFAULT(TObjectsCountByKeyIndex, TPolicy, Policy);
public:
    const TMap<TKey, i32>& GetIndexData() const {
        return IndexedObjects;
    }

    i32 GetObjectsCountByKey(const TKey& key) const {
        auto it = IndexedObjects.find(key);
        if (it == IndexedObjects.end()) {
            return 0;
        }
        return it->second;
    }

    template <class TId, class TDerivObject>
    bool Initialize(const TMap<TId, TDerivObject>& objects) {
        IndexedObjects.clear();
        for (auto&& i : objects) {
            const auto& key = Policy.GetKey(i.second);
            if (!key) {
                continue;
            }
            IndexedObjects[key]++;
            CurrentKeyForObject.emplace(Policy.GetUniqueId(i.second), key);
        }
        return true;
    }

    bool Upsert(const TObject& object) {
        const auto& newObjectKey = Policy.GetKey(object);
        if (!newObjectKey) {
            return Remove(object);
        }

        auto objectId = Policy.GetUniqueId(object);
        auto itCurrentKey = CurrentKeyForObject.find(objectId);
        if (itCurrentKey != CurrentKeyForObject.end()) {
            auto currentIndex = IndexedObjects.find(itCurrentKey->second);
            CHECK_WITH_LOG(currentIndex != IndexedObjects.end());
            if (newObjectKey != itCurrentKey->second) {
                --currentIndex->second;
            } else {
                return true;
            }
            itCurrentKey->second = newObjectKey;
        } else {
            CurrentKeyForObject.emplace(objectId, newObjectKey);
        }
        auto it = IndexedObjects.find(newObjectKey);
        if (it == IndexedObjects.end()) {
            IndexedObjects.emplace(newObjectKey, 1);
        } else {
            ++it->second;
        }
        return true;
    }
    bool Remove(const TObject& object) {
        auto objectId = Policy.GetUniqueId(object);
        auto itCurrentKey = CurrentKeyForObject.find(objectId);
        if (itCurrentKey == CurrentKeyForObject.end()) {
            return true;
        }
        auto it = IndexedObjects.find(itCurrentKey->second);
        CHECK_WITH_LOG(it != IndexedObjects.end());
        --it->second;
        if (!it->second) {
            IndexedObjects.erase(it);
        }
        CurrentKeyForObject.erase(itCurrentKey);
        return true;
    }
};

template <class TEntity, class TIdType = TString>
class TDBEntitiesCacheImpl {
    using TIdRef = typename TIdTypeSelector<TIdType>::TIdRef;
protected:
    TRWMutex MutexCachedObjects;
    mutable TMap<TIdType, TEntity> Objects;

private:
    template <bool Detach, class TAction, class TObjects>
    bool ForObjectsListImpl(TObjects& objects, const TSet<TIdType>* selectedObjectIds, TAction& action) const {
        if (Detach) {
            const auto actionImpl = [&action](const TIdRef& /*id*/, TEntity&& entity) {
                action(std::move(entity));
            };
            return ForObjectsMapImpl<Detach>(objects, selectedObjectIds, actionImpl);
        } else {
            const auto actionImpl = [&action](const TIdRef& /*id*/, const TEntity& entity) {
                action(entity);
            };
            return ForObjectsMapImpl<Detach>(objects, selectedObjectIds, actionImpl);
        }
    }

    template <bool Detach, class TAction, class TObjects>
    bool ForObjectsMapImpl(TObjects& objects, const TSet<TIdType>* selectedObjectIds, TAction& action) const {
        if (!!selectedObjectIds && selectedObjectIds->empty()) {
            return true;
        }
        if (!selectedObjectIds) {
            for (auto&& obj : objects) {
                action(obj.first, Detach ? std::move(obj.second) : obj.second);
            }
        } else {
            if (selectedObjectIds->size() < objects.size() / 20) {
                for (auto&& i : *selectedObjectIds) {
                    auto it = objects.find(i);
                    if (it != objects.end()) {
                        action(it->first, Detach ? std::move(it->second) : it->second);
                    }
                }
            } else {
                auto itId = selectedObjectIds->begin();
                auto itObj = objects.begin();
                while (itId != selectedObjectIds->end() && itObj != objects.end()) {
                    if (*itId < itObj->first) {
                        ++itId;
                    } else if (*itId > itObj->first) {
                        ++itObj;
                    } else {
                        action(itObj->first, Detach ? std::move(itObj->second) : itObj->second);
                        ++itId;
                        ++itObj;
                    }
                }
            }
        }
        return true;
    }

public:
    virtual bool RefreshCache(const TInstant reqActuality, const bool doActualizeHistory = true) const = 0;
    virtual ~TDBEntitiesCacheImpl() {}

    TMaybe<TEntity> GetObject(const TIdType& id, const TInstant reqActuality = TInstant::Zero()) const {
        TVector<TEntity> result;
        GetCustomObjects(result, { id }, reqActuality);
        return result.size() ? result.front() : TMaybe<TEntity>();
    }

    TEntity GetObjectDef(const TIdType& id, const TInstant reqActuality = TInstant::Zero()) const {
        TMaybe<TEntity> result = GetObject(id, reqActuality);
        if (!!result) {
            return *result;
        } else {
            return Default<TEntity>();
        }
    }

    bool HasObjects(const TInstant reqActuality = TInstant::Zero()) const {
        if (!RefreshCache(reqActuality)) {
            ALERT_LOG << "Cache refresh failed" << Endl;
        }
        TReadGuard rg(MutexCachedObjects);
        return Objects.size();
    }

    template<class TContainer>
    bool GetCachedObjectsVector(const TContainer& ids, TVector<TEntity>& result, const TInstant reqActuality) const {
        if (!RefreshCache(reqActuality)) {
            return false;
        }
        TReadGuard rg(MutexCachedObjects);
        for (auto&& id : ids) {
            auto objectIt = Objects.find(id);
            if (objectIt == Objects.end()) {
                continue;
            }
            result.emplace_back(objectIt->second);
        }
        return true;
    }

    TVector<TEntity> GetCachedObjectsVector(const TInstant reqActuality = TInstant::Zero()) const {
        RefreshCache(reqActuality);

        TVector<TEntity> result;
        TReadGuard rg(MutexCachedObjects);
        for (auto&& i : Objects) {
            result.emplace_back(i.second);
        }
        return result;
    }

    TVector<TEntity> GetCachedObjectsVector(const std::function<bool(const TEntity&)> checker, const TInstant reqActuality = TInstant::Zero()) const {
        RefreshCache(reqActuality);

        TVector<TEntity> result;
        TReadGuard rg(MutexCachedObjects);
        for (auto&& i : Objects) {
            if (checker(i.second)) {
                result.emplace_back(i.second);
            }
        }
        return result;
    }

    template <class TContainer>
    bool GetCustomObjects(const TContainer& ids, TMap<TIdType, TEntity>& result, const TInstant reqActuality) const {
        const auto action = [&result](const TIdType& key, const TEntity& entity) {
            result.emplace(key, entity);
        };
        return ForObjectsMap(action, reqActuality, &ids);
    }

    TMap<TIdType, TEntity> GetCachedObjectsMap() const {
        TReadGuard rg(MutexCachedObjects);
        return Objects;
    }

    template<class TContainer>
    TMap<TIdType, TEntity> GetCachedObjectsMap(const TContainer& ids, const TInstant reqActuality = TInstant::Zero()) const {
        RefreshCache(reqActuality);
        TReadGuard rg(MutexCachedObjects);
        TMap<TIdType, TEntity> result;
        for (auto&& id : ids) {
            auto objectIt = Objects.find(id);
            if (objectIt == Objects.end()) {
                continue;
            }
            result.emplace(id, objectIt->second);
        }
        return result;
    }

    bool GetCustomObjects(TVector<TEntity>& result, const TSet<TIdType>& ids, const TInstant reqActuality = TInstant::Zero()) const {
        const auto action = [&result](const TIdType& /*key*/, const TEntity& entity) {
            result.emplace_back(entity);
        };
        return ForObjectsMap(action, reqActuality, &ids);
    }

    bool GetAllObjects(TVector<TEntity>& result, const TInstant reqActuality = TInstant::Zero()) const {
        if (!RefreshCache(reqActuality)) {
            return false;
        }
        TReadGuard rg(MutexCachedObjects);
        for (auto&& obj : Objects) {
            TEntity eFiltered(obj.second);
            result.emplace_back(std::move(eFiltered));
        }
        return true;
    }

    bool GetObjectIds(TSet<TIdType>& result, const TInstant reqActuality = TInstant::Zero()) const {
        if (!RefreshCache(reqActuality)) {
            return false;
        }
        TReadGuard rg(MutexCachedObjects);
        for (auto&& obj : Objects) {
            result.emplace(obj.first);
        }
        return true;
    }

    TSet<TIdType> GetObjectIds() const {
        TSet<TIdType> result;
        TReadGuard rg(MutexCachedObjects);
        for (auto&& obj : Objects) {
            result.emplace(obj.first);
        }
        return result;
    }

    bool GetAllObjects(TMap<TIdType, TEntity>& result, const TInstant reqActuality = TInstant::Zero()) const {
        if (!RefreshCache(reqActuality)) {
            return false;
        }
        TReadGuard rg(MutexCachedObjects);
        for (auto&& obj : Objects) {
            TEntity eFiltered(obj.second);
            result.emplace(obj.first, std::move(eFiltered));
        }
        return true;
    }

    template <class TAction>
    bool ForObjectsList(TAction& action, const TInstant reqActuality, const TSet<TIdType>* objectIds = nullptr) const {
        if (!RefreshCache(reqActuality)) {
            return false;
        }
        TReadGuard rg(MutexCachedObjects);
        return ForObjectsListImpl<false>(Objects, objectIds, action);
    }

    template <class TAction>
    bool ForObjectsMap(TAction& action, const TInstant reqActuality, const TSet<TIdType>* objectIds = nullptr) const {
        if (!RefreshCache(reqActuality)) {
            return false;
        }
        TReadGuard rg(MutexCachedObjects);
        return ForObjectsMapImpl<false>(Objects, objectIds, action);
    }

    template <class TAction>
    bool ForDetachedObjectsList(TAction& action, const TInstant reqActuality, const TSet<TIdType>* objectIds = nullptr) const {
        if (!RefreshCache(reqActuality)) {
            return false;
        }
        TMap<TIdType, TEntity> detached;
        {
            TReadGuard rg(MutexCachedObjects);
            if (objectIds && objectIds->size() < Objects.size() / 20) {
                for (auto&& i : *objectIds) {
                    auto it = Objects.find(i);
                    if (it != Objects.end()) {
                        detached.emplace(i, it->second);
                    }
                }
            } else {
                detached = Objects;
            }
        }
        return ForObjectsListImpl<true>(detached, objectIds, action);
    }
};

namespace NCommonInterface {
    class TCommonDecoder: public TBaseDecoder {
    private:
        using TBase = TBaseDecoder;
        CS_ACCESS(TCommonDecoder, i32, Data, -1);
    public:
        TCommonDecoder() = default;
        TCommonDecoder(const TMap<TString, ui32>& decoderBase) {
            Data = GetFieldDecodeIndex("data", decoderBase);
        }
    };
}

template <class TProto>
class IProtoDecoderSerializable: public INativeProtoSerialization<TProto> {
private:
    using TBase = INativeProtoSerialization<TProto>;
public:
    using TBase::SerializeToProto;
    using TBase::DeserializeFromProto;
    virtual ~IProtoDecoderSerializable() = default;

    using TDecoder = NCommonInterface::TCommonDecoder;

    Y_WARN_UNUSED_RESULT virtual bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) final {
        TProto proto;
        if (!decoder.GetProtoValueBytes(decoder.GetData(), values, proto)) {
            return false;
        }
        return DeserializeFromProto(proto);
    }
    virtual NStorage::TTableRecord SerializeToTableRecord() const final {
        NStorage::TTableRecord result;
        TProto proto;
        SerializeToProto(proto);
        result.SetProtoBytes("data", std::move(proto));
        return result;
    }
};

class IStringDecoderSerializable {
public:
    virtual TString SerializeToString() const = 0;
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromString(const TString& info) = 0;
    virtual ~IStringDecoderSerializable() = default;
    using TDecoder = NCommonInterface::TCommonDecoder;

    Y_WARN_UNUSED_RESULT virtual bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) final {
        TString data;
        READ_DECODER_VALUE_TEMP(decoder, values, data, Data);
        return DeserializeFromString(data);
    }
    virtual NStorage::TTableRecord SerializeToTableRecord() const final {
        NStorage::TTableRecord result;
        result.Set("data", SerializeToString());
        return result;
    }

};

class IJsonDecoderSerializable {
public:
    virtual NJson::TJsonValue SerializeToJson() const = 0;
    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
    virtual ~IJsonDecoderSerializable() = default;

    using TDecoder = NCommonInterface::TCommonDecoder;

    bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        TString data;
        READ_DECODER_VALUE_TEMP(decoder, values, data, Data);
        NJson::TJsonValue jsonData;
        if (!NJson::ReadJsonFastTree(data, &jsonData)) {
            TFLEventLog::Error("cannot parse string as json");
            return false;
        }
        return DeserializeFromJson(jsonData);
    }
    NStorage::TTableRecord SerializeToTableRecord() const {
        NStorage::TTableRecord result;
        result.Set("data", SerializeToJson());
        return result;
    }

};

template <class TInterface, class TConfiguration = TDefaultInterfaceContainerConfiguration>
class TInterfaceContainer: public TBaseInterfaceContainer<TInterface, TConfiguration> {
private:
    using TBase = TBaseInterfaceContainer<TInterface, TConfiguration>;
protected:
    using TBase::Object;
public:
    using TBase::TBase;

    class TDecoder: public TInterface::TDecoder {
    private:
        using TBase = typename TInterface::TDecoder;
        DECODER_FIELD(ClassName);
    public:
        TDecoder() = default;
        TDecoder(const TMap<TString, ui32>& decoderBase)
            : TBase(decoderBase) {
            ClassName = TBase::GetFieldDecodeIndex(TConfiguration::GetClassNameField(), decoderBase);
        }
    };

    TInterfaceContainer() = default;

    NStorage::TTableRecord SerializeToTableRecord() const {
        NStorage::TTableRecord result;
        if (Object) {
            result = Object->SerializeToTableRecord();
        }
        result.Set(TConfiguration::GetClassNameField(), TBase::GetClassName());
        return result;
    }

    bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        TString className;
        READ_DECODER_VALUE_TEMP(decoder, values, className, ClassName);
        if (!className) {
            TFLEventLog::Error("empty class name");
            return false;
        }
        THolder<TInterface> object(TInterface::TFactory::Construct(className));
        if (!object) {
            TFLEventLog::Error("incorrect class name")("class_name", className);
            return false;
        }
        if (!object->DeserializeWithDecoder(decoder, values)) {
            TFLEventLog::Error("cannot parse internal container object");
            return false;
        }
        Object = object.Release();
        return true;
    }
};
