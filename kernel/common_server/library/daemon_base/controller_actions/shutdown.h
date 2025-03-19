#pragma once
#include <kernel/common_server/library/daemon_base/actions_engine/controller_client.h>

#define SHUTDOWN_ACTION_NAME "SHUTDOWN"

namespace NDaemonController {

    class TShutdownAction : public TAction {
    private:
    protected:

        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual void DoDeserializeFromJson(const NJson::TJsonValue& json) override;

        static TFactory::TRegistrator<TShutdownAction> Registrator;
    public:
        virtual TString DoBuildCommand() const override {
            return "command=shutdown&async=yes";
        }

        virtual TDuration GetTimeoutDuration() const override {
            return TDuration::Seconds(60);
        }

        virtual void DoInterpretResult(const TString& result) override;

        virtual TString ActionName() const override { return SHUTDOWN_ACTION_NAME; }
    };
}
