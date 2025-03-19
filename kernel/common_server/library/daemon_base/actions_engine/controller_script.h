#pragma once

#include "controller_client.h"
#include <kernel/common_server/library/cluster/cluster.h>
#include <kernel/common_server/library/tasks_graph/script.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/json_reader.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>

namespace NRTYScript {

    struct TSlotAction: public ITask {

        class TSlotData : public NRTYCluster::TSlotData {
        public:
            TString UriPrefix;
            TSlotData() {

            }

            TSlotData(const TString& host, const ui32 port, const TString& uriPrefix)
                : NRTYCluster::TSlotData(host, port)
                , UriPrefix(uriPrefix)
            {

            }
        };

        TSlotData Slot;
        NDaemonController::TAction::TPtr Action;
        static TFactory::TRegistrator<TSlotAction> Registrator;

        virtual void TakeInfo(const ITasksInfo& info) override {
            Action->AddPrevActionsResult(info);
        }

        virtual void Fail(const TString& message) override {
            Action->Fail(message);
        }

        virtual bool Execute(void* externalInfo) const override;

        virtual TString GetClass() const override {
            return "slot_action";
        }

        virtual TString GetName() const override {
            return Action->ActionId() + "/" + Slot.ShortSlotName();
        }

        virtual NJson::TJsonValue Serialize() const override {
            NJson::TJsonValue result(NJson::JSON_MAP);
            result["slot"] = Slot.FullSlotName();
            result["uri_prefix"] = Slot.UriPrefix;
            result["action"] = Action->Serialize();
            return result;
        }

        virtual void Deserialize(const NJson::TJsonValue& info) override {
            NRTYCluster::TSlotData::Parse(info["slot"].GetStringRobust(), Slot);
            if (!info["uri_prefix"].GetString(&Slot.UriPrefix))
                Slot.UriPrefix = "";
            Action = NDaemonController::TAction::BuildAction(info["action"]);
        }
    };

    class TRTYClusterScript;

    struct TSlotTaskContext {
        TString Host;
        ui32 Port;
        TString UriPrefix;

        TSlotTaskContext() {

        }

        TSlotTaskContext(const TString& host, ui32 port, const TString& uriPrefix) {
            Host = host;
            Port = port;
            UriPrefix = uriPrefix;
        }
    };

    class TRTYClusterScript;

    class TSlotTaskContainer {
    private:
        TTaskContainer TaskContainer;
        TSlotTaskContext Context;
    public:
        TSlotTaskContainer(const TTaskContainer& taskContainer, const TString& host, ui32 port, const TString& uriPrefix)
            : Context(host, port, uriPrefix)
        {
            TaskContainer = taskContainer;
        }

        operator TTaskContainer() const {
            return TaskContainer;
        }

        const TSlotTaskContext& GetContext() const {
            return Context;
        }

        const TTaskContainer& GetContainer() const {
            return TaskContainer;
        }

        TString GetName() const {
            return TaskContainer.GetName();
        }

        template <class TChecker = TSucceededChecker>
        TSlotTaskContainer AddNext(TRTYClusterScript& script, NDaemonController::TAction::TPtr action);
    };

    class TContainersPool {
    private:
        TVector<TTaskContainer> Containers;
        TSlotTaskContext Context;
    public:
        bool IsEmpty() const {
            return !Containers.size();
        }

        void Add(const TSlotTaskContainer& container) {
            if (!Containers.size()) {
                Context = container.GetContext();
                Containers.push_back(container.GetContainer());
            } else {
                CHECK_WITH_LOG(container.GetContext().Host == Context.Host);
                CHECK_WITH_LOG(container.GetContext().Port == Context.Port);
                CHECK_WITH_LOG(container.GetContext().UriPrefix == Context.UriPrefix);
                Containers.push_back(container.GetContainer());
            }
        }

        template <class TChecker = TSucceededChecker>
        TSlotTaskContainer AddNext(TRTYClusterScript& script, NDaemonController::TAction::TPtr action);

    };

    class TRTYClusterScript: public TScript {
    private:
        TString TaskName;
        TString Parent;
    public:

        typedef TAtomicSharedPtr<TRTYClusterScript> TPtr;

        virtual NJson::TJsonValue Serialize() const override {
            NJson::TJsonValue result = TScript::Serialize();
            result["task_name"] = TaskName;
            result["parent"] = Parent;
            return result;
        }

        virtual bool Deserialize(const NJson::TJsonValue& info) override {
            if (!TScript::Deserialize(info))
                return false;
            TaskName = info["task_name"].GetStringRobust();
            Parent = info["parent"].GetStringRobust();
            return true;
        }

        TRTYClusterScript(const TString& taskName, const TString& parent) {
            TaskName = taskName;
            Parent = parent;
        }

        TRTYClusterScript(IController::TPtr controller = nullptr)
            : TScript(controller)
        {
        }

        TString GetTaskPath() const {
            return Parent + "/" + TaskName;
        }

        TSlotTaskContainer AddAction(const TString& host, ui32 port, const TString& uriPrefix, NDaemonController::TAction::TPtr action) {
            TAutoPtr<TSlotAction> task(new TSlotAction);
            task->Action = action;
            task->Slot = NRTYScript::TSlotAction::TSlotData(host, port, uriPrefix);
            action->SetParent(Parent + "/" + TaskName);
            return TSlotTaskContainer(TScript::AddTaskAfterAll(task.Release()), host, port, uriPrefix);
        }

        template <class TChecker = TFinishedChecker>
        TSlotTaskContainer AddDependentAction(const TString& host, ui32 port, const TString& uriPrefix, NDaemonController::TAction::TPtr action, const TVector<TTaskContainer>& dependTasks) {
            TAutoPtr<TSlotAction> task(new TSlotAction);
            task->Action = action;
            task->Slot = NRTYScript::TSlotAction::TSlotData(host, port, uriPrefix);
            action->SetParent(Parent + "/" + TaskName);
            TTaskContainer result = TScript::AddTask(task.Release());
            for (auto& i : dependTasks) {
                AddSeqInfo(i, result, new TChecker());
            }
            return TSlotTaskContainer(result, host, port, uriPrefix);
        }

        template <class TChecker = TFinishedChecker>
        TSlotTaskContainer AddDependentAction(const TString& host, ui32 port, const TString& uriPrefix, NDaemonController::TAction::TPtr action, const TTaskContainer& dependTask) {
            TVector<TTaskContainer> tasks;
            tasks.push_back(dependTask);
            return AddDependentAction<TChecker>(host, port, uriPrefix, action, tasks);
        }

        TSlotTaskContainer AddIndependentAction(const TString& host, ui32 port, const TString& uriPrefix, NDaemonController::TAction::TPtr action) {
            TAutoPtr<TSlotAction> task(new TSlotAction);
            task->Action = action;
            task->Slot = NRTYScript::TSlotAction::TSlotData(host, port, uriPrefix);
            action->SetParent(Parent + "/" + TaskName);
            return TSlotTaskContainer(TScript::AddTask(task.Release()), host, port, uriPrefix);
        }
    };

    template <class TChecker>
    TSlotTaskContainer TSlotTaskContainer::AddNext(TRTYClusterScript& script, NDaemonController::TAction::TPtr action) {
        if (!action)
            return *this;
        return script.AddDependentAction<TChecker>(Context.Host, Context.Port, Context.UriPrefix, action, TaskContainer);
    }

    template <class TChecker>
    TSlotTaskContainer TContainersPool::AddNext(TRTYClusterScript& script, NDaemonController::TAction::TPtr action) {
        CHECK_WITH_LOG(Containers.size());
        CHECK_WITH_LOG(!!action);
        return script.AddDependentAction<TChecker>(Context.Host, Context.Port, Context.UriPrefix, action, Containers);
    }
}

