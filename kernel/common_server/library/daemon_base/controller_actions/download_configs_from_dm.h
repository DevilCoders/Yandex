#pragma once

#include "async_controller_action.h"
#include <kernel/daemon/config/daemon_config.h>
#include <kernel/common_server/library/sharding/sharding.h>

namespace NDaemonController {

    class TDownloadConfigsFromDmAction : public TControllerAsyncAction {
    public:
        TDownloadConfigsFromDmAction();
        TDownloadConfigsFromDmAction(TAsyncPolicy policy, const TString& configVersion);
        TDownloadConfigsFromDmAction(const TString& waitActionName);

        virtual TLockType GetLockType() const override;
        virtual bool GetNotContinuableTaskOnStarting() const override;
        virtual TString ActionName() const override;
        TDaemonConfig::TControllerConfig::TDMOptions& MutableDMOptions();
        TDownloadConfigsFromDmAction& SetForce(const bool force);
        TDownloadConfigsFromDmAction& SetServiceType(const TString& serviceType);
        virtual void AddPrevActionsResult(const NRTYScript::ITasksInfo& info) override;
    protected:
        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual void DoDeserializeFromJson(const NJson::TJsonValue& json) override;
        virtual TString DoBuildCommandStart() const override;
        virtual void InterpretTaskReply(TAsyncTaskExecutor::TTask::TStatus taskStatus, const NJson::TJsonValue& result) override;

        static TFactory::TRegistrator<TDownloadConfigsFromDmAction> Registrator;

    private:
        TDaemonConfig::TControllerConfig::TDMOptions DMOptions;
        TString ServiceType;
        TString ConfigVersion;
        bool Force = false;
    };
}
