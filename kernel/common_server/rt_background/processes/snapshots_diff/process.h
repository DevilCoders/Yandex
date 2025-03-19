#pragma once

#include <kernel/common_server/rt_background/settings.h>
#include <kernel/common_server/rt_background/state.h>
#include <kernel/common_server/util/accessor.h>

namespace NCS {

    class TSnapshotsDiffProcess: public IRTRegularBackgroundProcess {
    private:
        using TBase = IRTRegularBackgroundProcess;
        static TFactory::TRegistrator<TSnapshotsDiffProcess> Registrator;
        CSA_DEFAULT(TSnapshotsDiffProcess, TString, TasksManagerId);
        CS_ACCESS(TSnapshotsDiffProcess, TString, QueueId, "snapshots_diff");
    protected:
        virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override {
            NFrontend::TScheme result = TBase::DoGetScheme(server);
            result.Add<TFSString>("tasks_manager_id");
            result.Add<TFSString>("queue_id");
            return result;
        }

        virtual TAtomicSharedPtr<IRTBackgroundProcessState> DoExecute(TAtomicSharedPtr<IRTBackgroundProcessState> state, const TExecutionContext& context) const override;

        virtual NJson::TJsonValue DoSerializeToJson() const override {
            auto result = TBase::DoSerializeToJson();
            result["tasks_manager_id"] = TasksManagerId;
            result["queue_id"] = QueueId;
            return result;
        }

        virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
            if (!TJsonProcessor::Read(jsonInfo, "tasks_manager_id", TasksManagerId, true, false)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "queue_id", QueueId)) {
                return false;
            }
            return TBase::DoDeserializeFromJson(jsonInfo);
        }
    public:
        virtual bool IsSimultaneousProcess() const override {
            return false;
        }

        static TString GetTypeName() {
            return "snapshots_diff";
        }

        virtual TString GetType() const override {
            return GetTypeName();
        }
    };

}
