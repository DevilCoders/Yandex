#pragma once
#include "async_controller_action.h"

#define RESTART_ACTION_NAME "RESTART"

namespace NDaemonController {

    class TRestartAction : public TControllerAsyncAction {
    private:
        enum TStage {stWaitSoftRestart, stWaitHardRestart};
        bool HardStop;
        TDuration StopTimeout;
        mutable TStage Stage;
        bool AbortIsFail;
        bool ParseJson(const TString& str, NJson::TJsonValue& result);
        void SetStage(TStage stage) const;
    protected:

        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual void DoDeserializeFromJson(const NJson::TJsonValue& json) override;

        virtual TString DoBuildCommandStart() const override;
        virtual TString DoBuildCommandWait() const override;

        virtual void DoInterpretResultWait(const TString& result) override;
        virtual void InterpretServerFailed() override;

        static TFactory::TRegistrator<TRestartAction> Registrator;
    public:

        TRestartAction()
            : HardStop(false)
            , StopTimeout(TDuration::Minutes(2))
            , Stage(stWaitSoftRestart)
            , AbortIsFail(false)
        {}

        TRestartAction(const TString& waitActionName, bool hardStop = false, bool abortIsFail = false)
            : TControllerAsyncAction(waitActionName)
            , HardStop(hardStop)
            , Stage(stWaitSoftRestart)
            , AbortIsFail(abortIsFail)
        {}

        TRestartAction(TAsyncPolicy asyncPolicy, bool hardStop = false, bool abortIsFail = false, const TDuration& stopTimeout = TDuration::Minutes(2))
            : TControllerAsyncAction(asyncPolicy)
            , HardStop(hardStop)
            , StopTimeout(stopTimeout)
            , Stage(stWaitSoftRestart)
            , AbortIsFail(abortIsFail)
        {}

        virtual TLockType GetLockType() const override {
            return ltWriteLock;
        }

        virtual TString ActionName() const override { return RESTART_ACTION_NAME; }
        virtual bool GetNotContinuableTaskOnStarting() const override;
        virtual ui32 GetAttemptionsMaxResend() const override;
    };
}
