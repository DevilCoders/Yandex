#include "script.h"
#include <library/cpp/balloc/optional/operators.h>
#include <library/cpp/logger/global/global.h>
#include <util/thread/pool.h>
#include <util/generic/buffer.h>
#include <util/string/vector.h>

namespace NRTYScript {

    class TScript::TTaskExecutor: public IObjectInQueue {
    private:
        TTaskContainer& Task;
        TScript& Script;
        TGraph::TVertexId Id;
    public:

        virtual ~TTaskExecutor() override {
            TGuard<TMutex> g(Script.Mutex);
            CHECK_WITH_LOG(Script.StartedTasks.contains(Id));
            Script.StartedTasks.erase(Script.StartedTasks.find(Id));
        }

        TTaskExecutor(TTaskContainer& task, TScript& script)
            : Task(task)
            , Script(script)
            , Id(Script.TasksSequence.NameToId(Task.GetName()))
        {
            Script.StartedTasks.insert(Id);
        }

        virtual void Process(void* /*ThreadSpecificResource*/) override {
            ThreadDisableBalloc();
            THolder<TTaskExecutor> this_(this);
            try {
                Task.GetTaskInfo().TakeInfo(Script);
                if (!!Script.Controller && !Script.Controller->IsExecutable())
                    return;
                Task.Execute(Script.ExternalInfo);
            } catch (...) {
                ERROR_LOG << "Can't execute task : " << Task.GetName() << "/" << CurrentExceptionMessage() << Endl;
            }
            CHECK_WITH_LOG(Task.IsFinished());
            Script.CheckNext(Task.GetName());
        }
    };

    bool TScript::IsFinished() const {
        TGuard<TMutex> g(Mutex);
        for (ui32 i = 0; i < TasksSequence.NumVertices(); ++i) {
            if (!TasksSequence.Vertex(i).IsFinished() && IsReadyForExecute(i)) {
                return false;
            }
        }
        return true;
    }

    bool TScript::IsSuccess() const {
        TGuard<TMutex> g(Mutex);
        CHECK_WITH_LOG(IsFinished());
        for (ui32 i = 0; i < TasksSequence.NumVertices(); ++i) {
            if (TasksSequence.Vertex(i).IsFailed()) {
                bool isFail = true;
                for (auto adj = TasksSequence.AdjacentIterator<NGraph::FORWARD>(i); adj.IsValid(); ++adj) {
                    if (adj.Edge().GetChecker()->Check(TasksSequence.Vertex(i))) {
                        isFail = false;
                    }
                }
                if (isFail)
                    return false;
            }
        }
        return true;
    }

    void TScript::CheckNext(const TString& name) {
        TGuard<TMutex> g(Mutex);
        CHECK_WITH_LOG(ReadOnly);
        TGraph::TVertexId id = TasksSequence.NameToId(name);
        CHECK_WITH_LOG(id != TGraph::NULL_VERTEX());
        CHECK_WITH_LOG(TasksSequence.Vertex(id).IsFinished());
        bool newTasks = false;
        for (auto adj = TasksSequence.AdjacentIterator<NGraph::FORWARD>(id); adj.IsValid(); ++adj) {
            if (IsReadyForExecute(adj.VertexId()) && adj.Edge().GetChecker()->Check(TasksSequence.Vertex(id))) {
                newTasks = true;
                ExecuteTask(adj.VertexId());
            }
        }
        if (!newTasks)
            CondVar.Signal();
    }

    void TScript::ExecuteTask(TGraph::TVertexId id) {
        TGuard<TMutex> g(Mutex);
        if (!StartedTasks.contains(id))
            Queue.SafeAdd(new TTaskExecutor(TasksSequence.Vertex(id), *this));
    }

    void TScript::Execute(ui32 threads, void* scriptInfo) {
        TGuard<TMutex> g(Mutex);
        ExternalInfo = scriptInfo;
        CHECK_WITH_LOG(!ReadOnly);
        ReadOnly = true;
        Queue.Start(threads);
        TVector<TGraph::TVertexId> firstStart;
        TSet<ui32> tasks;
        for (ui32 i = 0; i < TasksSequence.NumVertices(); ++i) {
            if (!TasksSequence.Vertex(i).IsFinished()) {
                if (IsReadyForExecute(i)) {
                    tasks.insert(i);
                }
            }
        }

        for (auto&& i : tasks) {
            ExecuteTask(i);
        }

        while (!StartedTasks.empty()) {
            CondVar.WaitT(Mutex, TDuration::Seconds(2));
        }
        Queue.Stop();

    }

    TTaskContainer TScript::AddTask(const ITask::TPtr task) {
        CHECK_WITH_LOG(!ReadOnly);
        TString name = "/" + Sprintf("%.4u", Size()) + "/" + task->GetName();
        for (ui32 i = 0; i < name.size(); ++i) {
            if (name[i] == '.' || name[i] == '-')
                name.replace(i, 1, "_");
        }

        TTaskContainer taskContainer(task, name);
        TasksSequence.AddVertex(taskContainer.GetName(), taskContainer);
        return TasksSequence.Vertex(taskContainer.GetName()).Props();
    }

    ui32 TScript::Size() const {
        return TasksSequence.NumVertices();
    }

    ui32 TScript::TasksCount(TTaskContainer::TStatus status) const {
        ui32 result = 0;
        for (ui32 i = 0; i < TasksSequence.NumVertices(); ++i) {
            if (TasksSequence.Vertex(i).GetStatus() & status)
                ++result;
        }
        return result;
    }

    TTaskContainer TScript::AddTaskAfterAll(const ITask::TPtr task) {
        auto result = AddTask(task);
        if (TasksSequence.NumVertices() > 1)
        for (ui32 i = 0; i < TasksSequence.NumVertices() - 1; ++i) {
            auto adj = TasksSequence.AdjacentIterator<NGraph::FORWARD>(i);
            if (!adj.IsValid()) {
                AddSeqInfo(TasksSequence.Vertex(i).Name, result.GetName(), new TFinishedChecker());
            }
        }
        return result;
    }

    TTaskContainer TScript::AddTaskLinear(const ITask::TPtr task) {
        auto result = AddTask(task);
        if (TasksSequence.NumVertices() > 1) {
            auto& lastVertex = TasksSequence.Vertex(TasksSequence.NumVertices() - 2);
            AddSeqInfo(lastVertex.Name, result.GetName(), new TFinishedChecker());
        }
        return result;
    }

    bool TScript::AddSeqInfo(const TString& from, const TString& to, IConnectionChecker::TPtr checker) {
        CHECK_WITH_LOG(!ReadOnly);
        TGraph::TVertexId idFrom = TasksSequence.NameToId(from);
        TGraph::TVertexId idTo = TasksSequence.NameToId(to);
        CHECK_WITH_LOG(TGraph::NULL_VERTEX() != idFrom && TGraph::NULL_VERTEX() != idTo);
        TasksSequence.AddEdge(from, to, checker);
        return true;
    }

    bool TScript::AddSeqInfo(const TTaskContainer& from, const TTaskContainer& to, IConnectionChecker::TPtr checker) {
        CHECK_WITH_LOG(!ReadOnly);
        TGraph::TVertexId idFrom = TasksSequence.NameToId(from.GetName());
        TGraph::TVertexId idTo = TasksSequence.NameToId(to.GetName());
        CHECK_WITH_LOG(TGraph::NULL_VERTEX() != idFrom && TGraph::NULL_VERTEX() != idTo);
        TasksSequence.AddEdge(from.GetName(), to.GetName(), checker);
        return true;
    }

    bool TScript::IsReadyForExecute(TGraph::TVertexId id) const {
        CHECK_WITH_LOG(ReadOnly);
        TGraph::TConstAdjacentIterator<NGraph::BACKWARD> adj = TasksSequence.AdjacentIterator<NGraph::BACKWARD>(id);
        ui32 finishedEdges = 0;
        ui32 edgesForContinue = 0;
        for (; adj.IsValid(); ++adj) {
            if (adj.Edge().GetChecker()->IsValuableForReady())
                ++edgesForContinue;
            if (adj.Edge().GetChecker()->Check(*adj)) {
                ++finishedEdges;
            }
        }
        CHECK_WITH_LOG(!edgesForContinue || (finishedEdges <= edgesForContinue));
        return !edgesForContinue || (finishedEdges == edgesForContinue);
    }

    bool TScript::Deserialize(const NJson::TJsonValue& info) {
        try {
            TasksSequence.DeserializeFromJson(info["graph"]);
        } catch (...) {
            ERROR_LOG << "Incorrect graph description format: " << info.GetStringRobust() << "/" << CurrentExceptionMessage() << Endl;
            return false;
        }
        return true;
    }

}
