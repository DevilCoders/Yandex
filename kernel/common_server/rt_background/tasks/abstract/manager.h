#pragma once
#include "object.h"
#include <library/cpp/object_factory/object_factory.h>

namespace NCS {
    namespace NBackground {
        class ITasksManager;

        class ITasksManagerConfig {
        private:
            CSA_READONLY_DEF(TString, TasksManagerId);
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) = 0;
            virtual void DoToString(IOutputStream& os) const = 0;
        public:
            using TPtr = TAtomicSharedPtr<ITasksManagerConfig>;
            using TFactory = NObjectFactory::TObjectFactory<ITasksManagerConfig, TString>;
            virtual ~ITasksManagerConfig() = default;
            virtual TAtomicSharedPtr<ITasksManager> Construct(const IBaseServer& server) const = 0;
            void Init(const TYandexConfig::Section* section) {
                TasksManagerId = section->Name;
                DoInit(section);
            }
            void ToString(IOutputStream& os) const {
                DoToString(os);
            }
            virtual TString GetClassName() const = 0;
        };

        class TTasksManagerConfigContainer: public TBaseInterfaceContainer<ITasksManagerConfig> {
        private:
            CSA_READONLY_DEF(TString, TasksManagerId);
            using TBase = TBaseInterfaceContainer<ITasksManagerConfig>;
        public:


            void Init(const TYandexConfig::Section* section) {
                TBase::Init(section);
                TasksManagerId = section->Name;
            }

            void ToString(IOutputStream& os) const {
                os << "<" << TasksManagerId << ">" << Endl;
                TBase::ToString(os);
                os << "</" << TasksManagerId << ">" << Endl;
            }

            using TBase::TBase;
        };

        class ITasksManager: public IAutoActualization {
        private:
            using TBase = IAutoActualization;
        protected:
            const TString TasksManagerId;
            const IBaseServer& Server;
            virtual bool DoAddTasks(const TVector<TRTQueueTaskContainer>& task) const = 0;
            virtual bool DoRestoreOwnerTasks(const TString& queueId, const TString& ownerId, TVector<TRTQueueTaskContainer>& result, const bool useReadReplica) const = 0;
            virtual bool DoRemoveTask(const TRTQueueTaskContainer& task) const = 0;
            virtual bool DoDropTaskExecution(const TRTQueueTaskContainer& task) const = 0;
            virtual bool DoGetTasksForExecution(const TString& queueId, const ui32 tasksLimit, TVector<TRTQueueTaskContainer>& tasks) const = 0;

            virtual bool Refresh() override {
                return true;
            }

            virtual bool DoStart() override {
                return TBase::DoStart();
            }

            virtual bool DoStop() override {
                return TBase::DoStop();
            }
        public:
            using TPtr = TAtomicSharedPtr<ITasksManager>;
            ITasksManager(const TString& tasksManagerId, const IBaseServer& server)
                : TBase("actualizer-" + tasksManagerId)
                , TasksManagerId(tasksManagerId)
                , Server(server) {

            }

            bool AddTasks(const TVector<TRTQueueTaskContainer>& tasks) const {
                auto gLogging = TFLRecords::StartContext()("&action", "task_enqueue")("&tasks_manager_id", TasksManagerId);
                const bool resultFlag = DoAddTasks(tasks);
                for (auto&& i : tasks) {
                    TFLEventLog::JustSignal("rt_background_tasks_manager")("&code", resultFlag ? "success" : "fail")("&task_class", i.GetClassName())(i.GetLogRecord());
                }
                return resultFlag;
            }
            bool RestoreCurrentTaskIds(const TString& queueId, const TString& ownerId, TSet<TString>& currentTaskIds) const;
            bool RestoreOwnerTasks(const TString& queueId, const TString& ownerId, TVector<TRTQueueTaskContainer>& result, const bool useReadReplica = false) const {
                auto gLogging = TFLRecords::StartContext()("&action", "ask_ownered_tasks")("&tasks_manager_id", TasksManagerId)("owner_id", ownerId)("queue_id", queueId);
                return DoRestoreOwnerTasks(queueId, ownerId, result, useReadReplica);
            }
            bool RemoveTask(const TRTQueueTaskContainer& task) const {
                auto gLogging = TFLRecords::StartContext()("&action", "remove_task")("&task_class", task.GetClassName())("&tasks_manager_id", TasksManagerId)(task.GetLogRecord());
                const bool resultFlag = DoRemoveTask(task);
                TFLEventLog::JustSignal("rt_background_tasks_manager")("&code", resultFlag ? "success" : "fail")(task.GetLogRecord());
                return resultFlag;
            }
            bool DropTaskExecution(const TRTQueueTaskContainer& task) const {
                auto gLogging = TFLRecords::StartContext()("&action", "drop_task_execution")("&tasks_manager_id", TasksManagerId)(task.GetLogRecord());
                const bool resultFlag = DoDropTaskExecution(task);
                TFLEventLog::Info()(task.GetLogRecord()).Signal("rt_background_tasks_manager")("&code", resultFlag ? "success" : "fail");
                return resultFlag;
            }
            bool GetTasksForExecution(const TString& queueId, const ui32 tasksLimit, TVector<TRTQueueTaskContainer>& tasks) const {
                auto gLogging = TFLRecords::StartContext()("&action", "get_tasks_for_execution")("&tasks_manager_id", TasksManagerId)("tasks_limit", tasksLimit)("&queue_id", queueId);
                const bool resultFlag = DoGetTasksForExecution(queueId, tasksLimit, tasks);
                for (auto&& i : tasks) {
                    TFLEventLog::JustSignal("rt_background_tasks_manager")("&code", resultFlag ? "success" : "fail")(i.GetLogRecord());
                }
                return resultFlag;
            }
        };
    }
}
