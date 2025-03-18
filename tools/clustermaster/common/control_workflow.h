#pragma once

#include "workflow.h"

#include <library/cpp/deprecated/transgene/transgene.h>

#include <util/generic/ptr.h>
#include <util/network/pollerimpl.h>
#include <util/network/sock.h>
#include <util/system/mutex.h>

class TControlWorkflow : public IWorkflow {
private:
    using TTransceiver = NTransgene::TTransceiver<TMutex>;
    using TPoller = TPollerImpl<TWithoutLocking>;

    struct TMessageProcessor {
        TControlWorkflow& ControlWorkflow;

        TMessageProcessor(TControlWorkflow& controlWorkflow) noexcept
            : ControlWorkflow(controlWorkflow)
        {
        }

        inline void operator()(TAutoPtr<NTransgene::TPackedMessage> what) const { ControlWorkflow.ProcessMessage(what); }
    };

public:
    virtual ~TControlWorkflow();

    void EnqueueMessage(TAutoPtr<NTransgene::TPackedMessage> message) noexcept {
        Transceiver.Enqueue(std::move(message));
    }
    void Run() noexcept final;

protected:
    TControlWorkflow(THolder<TStreamSocket> socket);

private:
    virtual void Greeting() = 0;
    virtual void ProcessMessage(TAutoPtr<NTransgene::TPackedMessage> what) = 0;
    virtual void OnClose() = 0;
    virtual int GetNetworkHeartbeat() const = 0;

private:
    THolder<TStreamSocket> Socket;
    TTransceiver Transceiver;

protected:
    TString MasterAddr;
};
