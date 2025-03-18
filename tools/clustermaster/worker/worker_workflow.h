#pragma once

#include "control_workflow.h"
#include "log.h"
#include "messages.h"

class TWorkerGraph;

class TWorkerControlWorkflow final : public TControlWorkflow {
public:
    TWorkerControlWorkflow(TWorkerGraph& tg, THolder<TStreamSocket> socket);

    bool Authorized() const noexcept { return AuthOk; }
    bool Initialized() const noexcept { return InitOk; }
    TString GetMasterHost() const { return MasterHost; }
    TIpPort GetMasterHttpPort() const noexcept { return MasterHttpPort; }

    bool IsPrimary() const noexcept {
        return !MasterIsSecondary;
    }

    template <typename TMessageSubclass>
    void EnqueueMessage(const TMessageSubclass& message) {
        DEBUGLOG1(net, GetMasterHost() << ": Enqueueing " << DebugStringForLog(message));
        EnqueueMessageImpl(message.Pack());
    }

private:
    void EnqueueMessageImpl(TAutoPtr<NTransgene::TPackedMessage> message) {
        TControlWorkflow::EnqueueMessage(std::move(message));
    }

    void Greeting() override;
    void ProcessMessage(TAutoPtr<NTransgene::TPackedMessage> what) override;
    void OnClose() override;
    int GetNetworkHeartbeat() const override;

private:
    TWorkerGraph& TargetGraph;
    bool AuthOk;
    bool InitOk;
    bool MasterIsSecondary;
    TString ExpectedAuthReply;
    TString MasterHost;
    TIpPort MasterHttpPort;
};
