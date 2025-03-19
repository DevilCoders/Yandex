#pragma once
#include "manager.h"
#include <kernel/common_server/rt_background/tasks/abstract/manager.h>
#include <kernel/common_server/library/vname_checker/checker.h>

namespace NCS {
    namespace NBackground {
        class TDBTasksManagerConfig: public ITasksManagerConfig {
        private:
            TString DBName;
            TString TasksScope;
            static TFactory::TRegistrator<TDBTasksManagerConfig> Registrator;
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) override {
                DBName = section->GetDirectives().Value("DBName", DBName);
                TasksScope = section->GetDirectives().Value("TasksScope", TasksScope);
                if (!!TasksScope) {
                    AssertCorrectConfig(NCS::TVariableNameChecker::DefaultObjectId(TasksScope), "invalid table suffix");
                }
                AssertCorrectConfig(!!DBName, "empty db name for background tasks manager");
            }
            virtual void DoToString(IOutputStream& os) const override {
                os << "DBName: " << DBName << Endl;
                os << "TasksScope: " << TasksScope << Endl;
            }
        public:
            virtual TAtomicSharedPtr<ITasksManager> Construct(const IBaseServer& server) const override;

            TString GetTableName() const {
                if (TasksScope) {
                    return "cs_background_tasks_" + TasksScope;
                } else {
                    return "cs_background_tasks";
                }
            }

            static TString GetTypeName() {
                return "db";
            }
            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

        class TDBTasksManager: public ITasksManager {
        private:
            using TBase = ITasksManager;
            NCS::NStorage::IDatabase::TPtr Database;
            TDBTasksManagerConfig Config;
            const TString UniqueToken = TGUID::CreateTimebased().AsUuidString();
            mutable TRWMutex Mutex;
            mutable TRWMutex UpdateTasksMutex;
            mutable TMap<ui64, TInstant> WaitingTaskIds;
            TMap<TSignalTagsSet, ui32> SignalsInfo;
            TDuration GetPingTimeout(const TString& queueId) const;
            TDuration GetPingInterval() const;
            TString GetAgentToken() const;
        protected:
            bool UpdateTasksLock() const;
            void RemoveTaskWatching(const ui64 taskId) const;
            void AddTaskWatching(const ui64 taskId) const;
            TMap<ui64, TInstant> GetTaskWatchingIds() const;
            void UpdateWatchingTasks(const TSet<ui64>& taskIds, const TInstant actualInstant) const;

            virtual bool Refresh() override;

            virtual bool DoAddTasks(const TVector<TRTQueueTaskContainer>& tasks) const override;
            virtual bool DoRestoreOwnerTasks(const TString& queueId, const TString& ownerId, TVector<TRTQueueTaskContainer>& result, const bool useReadReplica) const override;
            virtual bool DoRemoveTask(const TRTQueueTaskContainer& task) const override;
            virtual bool DoDropTaskExecution(const TRTQueueTaskContainer& task) const override;
            virtual bool DoGetTasksForExecution(const TString& queueId, const ui32 tasksLimit, TVector<TRTQueueTaskContainer>& tasks) const override;
        public:
            TDBTasksManager(const TString& tasksManagerId, const IBaseServer& server, const TDBTasksManagerConfig& config, NCS::NStorage::IDatabase::TPtr db)
                : TBase(tasksManagerId, server)
                , Database(db)
                , Config(config)
            {
                CHECK_WITH_LOG(db);
            }

        };
    }
}
