#pragma once

#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/rt_background/state.h>
#include <kernel/common_server/util/accessor.h>

namespace NCS {

    class TQueueExecutorProcess: public IRTRegularBackgroundProcess {
    private:
        using TBase = IRTRegularBackgroundProcess;
        static TFactory::TRegistrator<TQueueExecutorProcess> Registrator;
        CSA_DEFAULT(TQueueExecutorProcess, TString, TasksManagerId);
        CSA_DEFAULT(TQueueExecutorProcess, TString, QueueName);
        CS_ACCESS(TQueueExecutorProcess, ui32, TasksExecutionLimit, 8);
        CS_ACCESS(TQueueExecutorProcess, bool, RemoveOnFail, false);
        CS_ACCESS(TQueueExecutorProcess, TDuration, MaxWorkingDuration, TDuration::Seconds(30));
    protected:
        virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;

        virtual TAtomicSharedPtr<IRTBackgroundProcessState> DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> state, const TExecutionContext& context) const override;

        virtual NJson::TJsonValue DoSerializeToJson() const override {
            auto result = TBase::DoSerializeToJson();
            result["tasks_manager_id"] = TasksManagerId;
            result["queue_name"] = QueueName;
            result["tasks_execution_limit"] = TasksExecutionLimit;
            result["remove_on_fail"] = RemoveOnFail;
            TJsonProcessor::WriteDurationString(result, "max_working_duration", MaxWorkingDuration);
            return result;
        }

        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
            if (!TJsonProcessor::Read(jsonInfo, "tasks_manager_id", TasksManagerId, true, false)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "queue_name", QueueName)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "remove_on_fail", RemoveOnFail)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "max_working_duration", MaxWorkingDuration)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "tasks_execution_limit", TasksExecutionLimit, true, false)) {
                return false;
            }
            return TBase::DoDeserializeFromJson(jsonInfo);
        }
    public:
        virtual bool IsSimultaneousProcess() const override {
            return true;
        }

        static TString GetTypeName() {
            return "queue_executor";
        }

        virtual TString GetType() const override {
            return GetTypeName();
        }
    };

}
