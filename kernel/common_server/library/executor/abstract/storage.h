#pragma once
#include <util/thread/pool.h>
#include <util/memory/blob.h>
#include <library/cpp/logger/global/global.h>
#include <kernel/common_server/library/executor/proto/task.pb.h>
#include <util/generic/buffer.h>
#include <util/stream/mem.h>
#include <util/generic/cast.h>
#include <kernel/common_server/library/storage/abstract.h>
#include <util/system/mutex.h>
#include "data.h"
#include "data_storage.h"
#include "task.h"

class IDistributedData;
class IDistributedTask;

class IDTasksStorage: public IDDataStorage {
private:
    TMutex MutexLocalTasks;
    mutable TMap<TString, TAtomicSharedPtr<IDistributedTask>> LocalTasks;
protected:
    virtual TVector<TString> GetNodes() const = 0;
public:

    void ClearOld(const TAtomic& active, const TDuration borderWaiting) const;

    bool IsLocalTask(const TString& taskId) const;

    using TPtr = TAtomicSharedPtr<IDTasksStorage>;

    virtual ~IDTasksStorage() = default;

    void RemoveTask(const TString& identifier) const;

    IDistributedTask::TPtr RestoreTaskUnsafe(const TString& identifier) const;
    IDistributedTask::TPtr RestoreTask(const TString& identifier) const;

    bool StoreInfo2(const TString& key, const TBlob& value) const {
        return StoreDataImpl2("info--" + key, value);
    }

    EReadStatus ReadInfo(const TString& key, TBlob& value) const {
        return ReadDataImpl("info--" + key, value);
    }
    bool StoreTask2(const IDistributedTask* data) const;
    void StoreLocalTask(IDistributedTask::TPtr data) const;
};

class IDTasksStorageConfig {
protected:
    virtual bool Init(const TYandexConfig::Section* section) = 0;
    virtual void DoToString(IOutputStream& os) const = 0;
public:
    using TFactory = NObjectFactory::TParametrizedObjectFactory<IDTasksStorageConfig, TString>;
    using TPtr = TAtomicSharedPtr<IDTasksStorageConfig>;

    virtual ~IDTasksStorageConfig() {

    }

    virtual TString GetName() const = 0;

    virtual void ToString(IOutputStream& os) const {
        os << "Type: " << GetName() << Endl;
        DoToString(os);
    }

    virtual IDTasksStorage::TPtr Construct() const = 0;

    static IDTasksStorageConfig::TPtr ConstructConfig(const TYandexConfig::Section* section) {
        TString type;
        AssertCorrectConfig(section->GetDirectives().GetValue("Type", type), "Can't read type for distributed queue in configuration");

        IDTasksStorageConfig::TPtr result = TFactory::Construct(type);
        AssertCorrectConfig(!!result, "Cannot construct config for type: '%s'", type.data());
        AssertCorrectConfig(result->Init(section), "Can't initialize queue from configuration for type: '%s'", type.data());
        return result;
    }
};
