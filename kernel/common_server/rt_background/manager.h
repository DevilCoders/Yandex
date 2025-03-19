#pragma once
#include "state.h"
#include "settings.h"
#include "config.h"
#include <kernel/common_server/library/storage/abstract.h>
#include <util/thread/pool.h>
#include <kernel/common_server/api/history/manager.h>
#include <kernel/common_server/api/history/cache.h>
#include <library/cpp/threading/named_lock/named_lock.h>
#include "storages/abstract.h"
#include "tasks/abstract/manager.h"
#include <kernel/common_server/util/algorithm/container.h>

class TRTBackgroundManager;

class TExecutionAgent: public IObjectInQueue, public IRTBackgroundProcess::ITaskExecutor {
private:
    using TStates = TRTBackgroundProcessStateContainer::TStates;
    TRTBackgroundProcessStateContainer ProcessState;
    TRTBackgroundProcessContainer ProcessSettings;
    NRTProc::TAbstractLock::TPtr ExternalLock;
    NNamedLock::TNamedLockPtr InternalLock;
    const IBaseServer& Server;
    NStorage::IDatabase::TPtr Database;
    const TRTBackgroundManagerConfig& Config;
    bool FlushState(const TRTBackgroundProcessStateContainer& stateContainer) const;
    bool FinishState(IRTBackgroundProcessState::TPtr state);
    TString GetLockName() const;

protected:
    virtual bool FlushStatus(const TString& status) override;
    virtual bool FlushActiveState(IRTBackgroundProcessState::TPtr state) override;
    bool CheckState(const TRTBackgroundProcessStateContainer& processState) const;

public:
    TExecutionAgent(TRTBackgroundProcessStateContainer&& processState, const TRTBackgroundProcessContainer& processSettings, const IBaseServer& server, NStorage::IDatabase::TPtr db, const TRTBackgroundManagerConfig& config)
        : ProcessState(std::move(processState))
        , ProcessSettings(processSettings)
        , Server(server)
        , Database(db)
        , Config(config)
    {

    }

    bool StartExecution(const TRTBackgroundManager& owner, TStates& states);

    virtual void Process(void* /*threadSpecificResource*/) override;
};

class TRTBackgroundManager: public IAutoActualization, public TNonCopyable {
private:
    using TBase = IAutoActualization;
    using TStates = TRTBackgroundProcessStateContainer::TStates;
    TThreadPool TasksQueue;
    const IBaseServer& Server;
    const TRTBackgroundManagerConfig Config;
    THolder<NBServer::IRTBackgroundProcessesStorage> Storage;
    TMap<TString, TAtomicSharedPtr<NCS::NBackground::ITasksManager>> TaskManagers;
    NStorage::IDatabase::TPtr Database;
    TRWMutex Mutex;
private:

    virtual bool Refresh() override;

    virtual bool DoStart() override {
        TasksQueue.Start(Config.GetThreadsCount());
        if (!Storage->StartStorage()) {
            return false;
        }
        for (auto&& tm : Config.GetTaskManagers()) {
            auto tasksManager = tm->Construct(Server);
            if (!tasksManager) {
                TFLEventLog::Error("cannot construct tasks manager")("tasks_manager_id", tm.GetTasksManagerId());
                return false;
            }
            if (!tasksManager->Start()) {
                return false;
            }
            TaskManagers.emplace(tm.GetTasksManagerId(), std::move(tasksManager));
        }
        return TBase::DoStart();
    }

    virtual bool DoStop() override {
        if (!TBase::DoStop()) {
            return false;
        }
        for (auto&& i : TaskManagers) {
            if (!i.second->Stop()) {
                return false;
            }
        }
        if (!Storage->StopStorage()) {
            return false;
        }
        TasksQueue.Stop();
        return true;
    }

public:
    TAtomicSharedPtr<NCS::NBackground::ITasksManager> GetTasksManager(const TString& tasksManagerId) const {
        auto it = TaskManagers.find(tasksManagerId);
        if (it == TaskManagers.end()) {
            return nullptr;
        }
        return it->second;
    }

    TSet<TString> GetTaskManagerIds() const {
        return MakeSet(NContainer::Keys(TaskManagers));
    }

    const NBServer::IRTBackgroundProcessesStorage& GetStorage() const {
        return *Storage;
    }

    static TRTBackgroundProcessStateContainer GetProcessState(const TStates& states, const TString& processName);

    bool WaitObjectInactive(const TString& processName, const TInstant deadline) const;

    bool GetStatesInfo(TMap<TString, TRTBackgroundProcessStateContainer>& states) const;
    bool RefreshState(TMap<TString, TRTBackgroundProcessStateContainer>& states, const TString& processName) const;

    TRTBackgroundManager(const IBaseServer& server, const TRTBackgroundManagerConfig& config)
        : TBase("TRTBackgroundManager::TTasksExecutor", config.GetPingPeriod())
        , Server(server)
        , Config(config)
        , Database(Server.GetDatabase(Config.GetDBName()))
    {
        AssertCorrectConfig(!!Database, "Incorrect DBName %s", Config.GetDBName().data());
        Storage = config.GetStorageConfig()->BuildStorage(Server);
        AssertCorrectConfig(!!Storage, "Cannot build rt-background storage");
    }

    ~TRTBackgroundManager() {
    }
};
