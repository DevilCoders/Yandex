#include "control_workflow.h"

#include "log.h"
#include "messages.h"
#include "thread_util.h"

#include <library/cpp/deprecated/fgood/fgood.h>

#include <util/generic/bt_exception.h>
#include <util/generic/singleton.h>
#include <util/stream/file.h>
#include <util/system/fs.h>

extern const NTransgene::TProtoVersion ProtoVersion;

namespace {
    constexpr TStringBuf UnknownAddress = "unknown";
}

TControlWorkflow::TControlWorkflow(THolder<TStreamSocket> socket)
    : Socket(std::move(socket))
    , Transceiver(ProtoVersion)
    , MasterAddr()
{
    Y_ENSURE(Socket != nullptr);
    LOG1(net, "Control workflow created");
}

TControlWorkflow::~TControlWorkflow() {
    LOG1(net, "Control workflow destroyed");
}

void TControlWorkflow::Run() noexcept {
    try {
        MasterAddr = UnknownAddress;
        char masterAddr[sizeof(in6_addr) * 5];
        if (GetRemoteAddr(*Socket, masterAddr, sizeof(masterAddr))) {
            MasterAddr = masterAddr;
        }

        LOG1(net, MasterAddr << ": Control workflow started");

        LOG1(net, MasterAddr << ": Sending hello message");
        Greeting();

        TPoller Poller;
        TPoller::TEvent event;

        while (true) {
            Poller.Set(nullptr, *Socket, CONT_POLL_READ | (Transceiver.HasDataToSend() ? CONT_POLL_WRITE : 0));

            if (!Poller.WaitD(&event, 1, TDuration::Seconds(GetNetworkHeartbeat()).ToDeadLine()))
                continue;

            if (TPoller::ExtractFilter(&event) & CONT_POLL_WRITE)
                Transceiver.Send(Socket.Get());

            if (TPoller::ExtractFilter(&event) & CONT_POLL_READ)
                Transceiver.Recv<const TMessageProcessor>(Socket.Get(), TMessageProcessor(*this));
        }
    } catch (const NTransgene::TDisconnection& /*e*/) {
        LOG1(net, MasterAddr << ": Master disconnected");
    } catch (const NTransgene::TBadVersion& e) {
        LOG1(net, MasterAddr << ": Bad master protocol version: " << e.what());
    } catch (...) {
        const auto currentExceptionMessage = CurrentExceptionMessage();
        LOG1(net, MasterAddr << ": Connection to master dropped, reason: " << currentExceptionMessage);
        LOG(currentExceptionMessage);
    }

    Socket->Close();

    DEBUGLOG("Closed socket");

    Transceiver.ResetState();
    Transceiver.ResetQueue();

    DEBUGLOG("Transceiver state was reseted");

    OnClose();

    LOG1(net, MasterAddr << ": Control workflow finished");
}
