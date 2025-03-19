#include "process.h"

#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/rt_background/manager.h>
#include <kernel/common_server/proto/background.pb.h>
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kernel/common_server/rt_background/tasks/abstract/object.h>
#include <util/random/random.h>

namespace NCS {

    TQueueExecutorProcess::TFactory::TRegistrator<TQueueExecutorProcess> TQueueExecutorProcess::Registrator(TQueueExecutorProcess::GetTypeName());

    NFrontend::TScheme TQueueExecutorProcess::DoGetScheme(const IBaseServer& server) const {
        NFrontend::TScheme result = TBase::DoGetScheme(server);
        result.Add<TFSVariants>("tasks_manager_id").SetVariants(server.GetRTBackgroundManager()->GetTaskManagerIds());
        result.Add<TFSString>("queue_name").SetRequired(false);
        result.Add<TFSString>("max_working_duration").SetDefault("30s").SetRequired(true);
        result.Add<TFSNumeric>("tasks_execution_limit").SetDefault(8);
        result.Add<TFSBoolean>("remove_on_fail").SetDefault(false);
        return result;
    }

    class TRTQueueExecutionContext {
    private:
        TMutex MutexCurrentTasks;
        TCondVar CondVarTasks;
        TAtomic CurrentTasks = 0;
        const IBaseServer& Server;
        NCS::NLogging::TBaseLogRecord LogContext;
    public:
        TRTQueueExecutionContext(const IBaseServer& server, NCS::NLogging::TBaseLogRecord&& logContext)
            : Server(server)
            , LogContext(logContext)
        {

        }

        const IBaseServer& GetServer() const {
            return Server;
        }

        NCS::NLogging::TLogThreadContext::TGuard StartLogging() const {
            return TFLRecords::StartContext()(LogContext);
        }

        i64 GetCurrentTasks() const {
            return AtomicGet(CurrentTasks);
        }

        void StartTask() {
            TGuard<TMutex> g(MutexCurrentTasks);
            AtomicIncrement(CurrentTasks);
            TFLEventLog::LSignal("tasks_execution", AtomicGet(CurrentTasks))("&code", "current_count").Debug();
            CondVarTasks.Signal();
        }

        void FinishTask() {
            TGuard<TMutex> g(MutexCurrentTasks);
            AtomicDecrement(CurrentTasks);
            TFLEventLog::LSignal("tasks_execution", AtomicGet(CurrentTasks))("&code", "current_count").Debug();
            CondVarTasks.Signal();
        }

        bool WaitTasks(const ui32 tasksCountLimit, ui32& askTasksCount) {
            TGuard<TMutex> g(MutexCurrentTasks);
            TFLEventLog::Debug("wait_task_started");
            while (tasksCountLimit <= AtomicGet(CurrentTasks) && Server.GetRTBackgroundManager()->IsActive()) {
                CondVarTasks.WaitT(MutexCurrentTasks, TDuration::Seconds(1));
            }
            if (Server.GetRTBackgroundManager()->IsActive()) {
                if (tasksCountLimit > AtomicGet(CurrentTasks)) {
                    askTasksCount = tasksCountLimit - AtomicGet(CurrentTasks);
                } else {
                    askTasksCount = 0;
                }
                TFLEventLog::Debug("wait_task_result")("count", askTasksCount);
                return true;
            }
            TFLEventLog::Debug("wait_task_finished_false");
            return false;
        }
    };

    class TRTQueueTaskExecutor: public IObjectInQueue {
    private:
        TRTQueueExecutionContext& Context;
        NBackground::ITasksManager::TPtr TasksManager;
        NBackground::TRTQueueTaskContainer Task;
        const bool RemoveOnFail;
    public:
        TRTQueueTaskExecutor(TRTQueueExecutionContext& context, NBackground::ITasksManager::TPtr tasksManager, NBackground::TRTQueueTaskContainer task, const bool removeOnFail)
            : Context(context)
            , TasksManager(tasksManager)
            , Task(task)
            , RemoveOnFail(removeOnFail)
        {

        }

        class TContextTaskGuard {
        private:
            TRTQueueExecutionContext& Context;
        public:
            TContextTaskGuard(TRTQueueExecutionContext& context)
                : Context(context) {
                Context.StartTask();
            }

            ~TContextTaskGuard() {
                Context.FinishTask();
            }
        };

        virtual void Process(void* /*threadSpecificResource*/) override {
            if (!Task || !*Task) {
                return;
            }
            auto gLogging = Context.StartLogging()
                ("&task_class", Task->GetClassName())("&action_class", (*Task)->GetClassName())("bg_task_id", Task->GetInternalTaskId())
                .AddSignalTags(Task->GetSignalTags());
            TContextTaskGuard ctg(Context);
            if (!(*Task)->Execute(*Task, Context.GetServer())) {
                if (RemoveOnFail) {
                    if (!TasksManager->RemoveTask(Task)) {
                        TFLEventLog::Error("task removing failed");
                    }
                } else {
                    if (!TasksManager->DropTaskExecution(Task)) {
                        TFLEventLog::Error("drop task execution failed");
                    }
                }
            } else if (!TasksManager->RemoveTask(Task)) {
                TFLEventLog::Error("ack message failed");
            }
        }
    };

    TAtomicSharedPtr<IRTBackgroundProcessState> TQueueExecutorProcess::DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> /*state*/, const TExecutionContext& context) const {
        const IBaseServer& server = context.GetServerAs<IBaseServer>();
        auto tasksManager = server.GetRTBackgroundManager()->GetTasksManager(TasksManagerId);
        if (!tasksManager) {
            TFLEventLog::Alert("In queue processor, nonexistent manager")("manager_id", GetTasksManagerId());
            return nullptr;
        }

        TRTQueueExecutionContext rtQueueContext(server, TFLRecords::Record());
        TThreadPool threads;
        threads.Start(GetTasksExecutionLimit());
        TFLEventLog::LSignal("tasks_execution", GetTasksExecutionLimit())("&code", "limit").Debug("start");

        ui32 askTasksCount;
        const TInstant startQueueWaiting = Now();
        while (Now() - startQueueWaiting < MaxWorkingDuration && rtQueueContext.WaitTasks(GetTasksExecutionLimit(), askTasksCount)) {
            TVector<NBackground::TRTQueueTaskContainer> tasks;
            TFLEventLog::Signal("tasks_execution", askTasksCount)("&code", "ask").Debug();
            if (!tasksManager->GetTasksForExecution(QueueName, askTasksCount, tasks)) {
                TFLEventLog::Signal("tasks_execution", askTasksCount)("&code", "ask_failed").Debug();
                Sleep(TDuration::MilliSeconds(1000 * RandomNumber<double>()));
                continue;
            } else {
                TFLEventLog::Signal("tasks_execution", tasks.size())("&code", "ask_success").Debug();
            }
            for (auto&& i : tasks) {
                threads.SafeAddAndOwn(MakeHolder<TRTQueueTaskExecutor>(rtQueueContext, tasksManager, i, RemoveOnFail));
            }
        }
        TFLEventLog::LSignal("tasks_execution", 0)("&code", "limit").Debug("finished");

        return MakeAtomicShared<IRTBackgroundProcessState>();
    }

}
