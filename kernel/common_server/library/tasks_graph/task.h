#pragma once

#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/object_factory/object_factory.h>

namespace NRTYScript {

    class ITasksInfo {
    public:
        virtual ~ITasksInfo() {};
        virtual NJson::TJsonValue GetTaskInfo(const TString& taskName) const = 0;
        bool GetValueByPath(const TString& taskName, const TString& path, NJson::TJsonValue& result) const;
        bool GetValueByPath(const TString& fullPath, NJson::TJsonValue& result) const;
    };

    class ITask {
    public:
        typedef NObjectFactory::TObjectFactory<ITask, TString> TFactory;
        typedef TAtomicSharedPtr<ITask> TPtr;
        virtual ~ITask() {};
    public:
        virtual void TakeInfo(const ITasksInfo& info) = 0;
        virtual bool Execute(void* externalInfo) const = 0;
        virtual TString GetClass() const = 0;
        virtual TString GetName() const = 0;
        virtual NJson::TJsonValue Serialize() const = 0;
        virtual void Deserialize(const NJson::TJsonValue& info) = 0;
        virtual void Fail(const TString& message) = 0;
    };

    class TTaskContainer {
    public:
        typedef ui64 TStatus;
    protected:
        mutable TStatus Status;
        ITask::TPtr Task;
        TString Name;
    public:

        static const TStatus StatusRunning = 1 << 0;
        static const TStatus StatusFailed = 1 << 1;
        static const TStatus StatusSuccess = 1 << 2;

        TTaskContainer(ITask::TPtr task, const TString& name) {
            Name = name;
            Task = task;
            Status = 0;
        }

        TTaskContainer() {
            Status = 0;
        }

        virtual ~TTaskContainer() {};

        template <class T>
        const T& GetTaskInfo() const {
            return dynamic_cast<const T&>(*Task);
        }

        const ITask& GetTaskInfo() const {
            return *Task;
        }

        ITask& GetTaskInfo() {
            return *Task;
        }

        TString GetName() const;

        TString GetStatusInfo() const;

        TStatus GetStatus() const {
            return Status;
        }

        bool HasStatus(TStatus status) const {
            return Status & status;
        }

        bool IsFinished() const {
            return (Status & StatusSuccess) || (Status & StatusFailed);
        }

        bool IsSuccess() const {
            return Status & StatusSuccess;
        }

        bool IsFailed() const {
            return Status & StatusFailed;
        }

        void TakeInfo(const ITasksInfo& info);

        void Execute(void* externalInfo) const;

        NJson::TJsonValue Serialize() const {
            NJson::TJsonValue result(NJson::JSON_MAP);
            result["class"] = Task->GetClass();
            result["status"] = Status;
            result["name"] = Name;
            result["task"] = Task->Serialize();
            return result;
        }

        void Deserialize(const NJson::TJsonValue& info);

    public:
    };

}
