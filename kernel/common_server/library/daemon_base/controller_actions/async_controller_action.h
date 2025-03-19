#pragma once

#include <kernel/common_server/library/daemon_base/actions_engine/controller_client.h>
#include <kernel/daemon/async/async_task_executor.h>

namespace NDaemonController {

    class TControllerAsyncAction : public TSimpleAsyncAction {
    private:
        bool ParseJson(const TString& str, NJson::TJsonValue& result);
    protected:

        virtual TString DoBuildCommandWait() const override;
        virtual void DoInterpretResultStart(const TString& result) override;
        virtual void DoInterpretResultWait(const TString& result) override;
        virtual void InterpretServerFailed();
        virtual void InterpretTaskReply(TAsyncTaskExecutor::TTask::TStatus taskStatus, const NJson::TJsonValue& result);

    public:

        TControllerAsyncAction()
        {}

        TControllerAsyncAction(const TString& waitActionName)
            : TSimpleAsyncAction(waitActionName)
        {}

        TControllerAsyncAction(TAsyncPolicy asyncPolicy)
            : TSimpleAsyncAction(asyncPolicy)
        {}
    };
}
