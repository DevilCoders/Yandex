#pragma once
#include <kernel/common_server/library/daemon_base/actions_engine/controller_client.h>

#define STATUS_ACTION_NAME "STATUS"

namespace NDaemonController {

    class TStatusAction : public TAction {
    private:
        TSet<TString> StatusAim;
        TSet<TString> ServerStatusAim;
    protected:

        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual void DoDeserializeFromJson(const NJson::TJsonValue& json) override;

        static TFactory::TRegistrator<TStatusAction> Registrator;
    public:

        TStatusAction() {
        }

        TStatusAction* AddVariantStatus(const TString& status) {
            StatusAim.insert(status);
            return this;
        }

        TStatusAction* AddVariantServerStatus(const TString& status) {
            ServerStatusAim.insert(status);
            return this;
        }

        virtual TString DoBuildCommand() const override {
            return "command=get_status";
        }

        virtual TDuration GetTimeoutDuration() const override {
            return TDuration::MilliSeconds(200);
        }

        virtual void DoInterpretResult(const TString& result) override;

        virtual TString ActionName() const override { return STATUS_ACTION_NAME; }
    };
}
