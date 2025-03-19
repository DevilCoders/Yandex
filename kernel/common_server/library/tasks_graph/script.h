#pragma once

#include <kernel/common_server/util/queue.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/graph/namedgraph.h>
#include <library/cpp/json/json_reader.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>
#include <util/string/cast.h>
#include <util/generic/set.h>
#include <library/cpp/digest/md5/md5.h>

#include "task.h"
#include "connection.h"

namespace NRTYScript {

    class IController {
    public:
        typedef TAtomicSharedPtr<IController> TPtr;
        virtual ~IController() {
        }
        virtual bool IsExecutable() = 0;
        virtual void Start() = 0;
        virtual void Finish() = 0;
        struct TLockOps {
            static void Acquire(TPtr* ptr) {
                if (*ptr)
                    (*ptr)->Start();
            }
            static void Release(TPtr* ptr) {
                if (*ptr)
                    (*ptr)->Finish();
            }
        };
        typedef TGuard<TPtr, TLockOps> TControllerGuard;
    };

    class TScript: public ITasksInfo {
    private:
        typedef NGraph::TNamedGraph<TString, TTaskContainer, TCheckerContainer> TGraph;
        TGraph TasksSequence;
        bool ReadOnly;
        TRTYMtpQueue Queue;
        TMutex Mutex;
        TCondVar CondVar;
        TSet<TGraph::TVertexId> StartedTasks;
        void* ExternalInfo;
        IController::TPtr Controller;
        class TTaskExecutor;
    private:
        bool IsReadyForExecute(TGraph::TVertexId id) const;

        void CheckNext(const TString& name);

        void ExecuteTask(TGraph::TVertexId id);

    public:

        virtual ~TScript() override {}

        TScript(IController::TPtr controller = nullptr)
            : Controller(controller)
        {
            ExternalInfo = nullptr;
            ReadOnly = false;
        }

        bool IsFinished() const;
        bool IsSuccess() const;

        TTaskContainer AddTask(const ITask::TPtr task);
        TTaskContainer AddTaskLinear(const ITask::TPtr task);
        TTaskContainer AddTaskAfterAll(const ITask::TPtr task);

        virtual NJson::TJsonValue GetTaskInfo(const TString& taskName) const override {
            TGraph::TVertexId id = TasksSequence.NameToId(taskName);
            if (id != TGraph::NULL_VERTEX()) {
                return TasksSequence.Vertex(id).Serialize()["task"];
            } else {
                NJson::TJsonValue result(NJson::JSON_MAP);
                return result;
            }
        }

        TString GetStatusInfo() const {
            return
                "success:" + ToString(TasksCount(TTaskContainer::StatusSuccess)) + ";" +
                "failed:" + ToString(TasksCount(TTaskContainer::StatusFailed)) + ";" +
                "running:" + ToString(TasksCount(TTaskContainer::StatusRunning)) + ";" +
                "all:" + ToString(Size()) + ";";
        }

        ui32 Size() const;
        ui32 TasksCount(TTaskContainer::TStatus status) const;

        bool AddSeqInfo(const TString& from, const TString& to, IConnectionChecker::TPtr checker);
        bool AddSeqInfo(const TTaskContainer& from, const TTaskContainer& to, IConnectionChecker::TPtr checker);

        void Execute(ui32 threads, void* scriptInfo = nullptr);

        TString BuildHash() const {
            return MD5::Calc(Serialize().GetStringRobust());
        }

        virtual NJson::TJsonValue Serialize() const {
            NJson::TJsonValue result;
            TasksSequence.SerializeToJson(result["graph"]);
            result["success"] = TasksCount(TTaskContainer::StatusSuccess);
            result["failed"] = TasksCount(TTaskContainer::StatusFailed);
            result["running"] = TasksCount(TTaskContainer::StatusRunning);

            for (ui32 i = 0; i < TasksSequence.NumVertices(); ++i) {
                if (TasksSequence.Vertex(i).GetStatus() & TTaskContainer::StatusFailed)
                    result["failed_tasks"].AppendValue(TasksSequence.Vertex(i).Serialize());
            }

            result["size"] = Size();
            return result;
        }

        virtual bool Deserialize(const NJson::TJsonValue& info);
    };
}
