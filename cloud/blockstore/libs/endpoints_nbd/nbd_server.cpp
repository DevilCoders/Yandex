#include "nbd_server.h"

#include <cloud/blockstore/libs/client/session.h>
#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/endpoints/endpoint_listener.h>
#include <cloud/blockstore/libs/nbd/server.h>
#include <cloud/blockstore/libs/nbd/server_handler.h>
#include <cloud/blockstore/libs/service/device_handler.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

namespace NCloud::NBlockStore::NServer {

using namespace NMonitoring;
using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TNbdEndpointListener final
    : public IEndpointListener
{
private:
    const NBD::IServerPtr Server;
    const ILoggingServicePtr Logging;
    const IServerStatsPtr ServerStats;

public:
    TNbdEndpointListener(
            NBD::IServerPtr server,
            ILoggingServicePtr logging,
            IServerStatsPtr serverStats)
        : Server(std::move(server))
        , Logging(std::move(logging))
        , ServerStats(std::move(serverStats))
    {}

    TFuture<NProto::TError> StartEndpoint(
        const NProto::TStartEndpointRequest& request,
        const NProto::TVolume& volume,
        NClient::ISessionPtr session) override
    {
        NBD::TStorageOptions options;
        options.DiskId = request.GetDiskId();
        options.ClientId = request.GetClientId();
        options.BlockSize = volume.GetBlockSize();
        options.BlocksCount = volume.GetBlocksCount();
        options.UnalignedRequestsDisabled = request.GetUnalignedRequestsDisabled();
        options.SendMinBlockSize = request.GetSendNbdMinBlockSize();

        auto requestFactory = CreateServerHandlerFactory(
            CreateDefaultDeviceHandlerFactory(),
            Logging,
            std::move(session),
            ServerStats,
            options);

        auto address = TNetworkAddress(
            TUnixSocketPath(request.GetUnixSocketPath()));

        return Server->StartEndpoint(address, std::move(requestFactory));
    }

    TFuture<NProto::TError> StopEndpoint(const TString& socketPath) override
    {
        auto address = TNetworkAddress(TUnixSocketPath(socketPath));

        return Server->StopEndpoint(address);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IEndpointListenerPtr CreateNbdEndpointListener(
    NBD::IServerPtr server,
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats)
{
    return std::make_shared<TNbdEndpointListener>(
        std::move(server),
        std::move(logging),
        std::move(serverStats));
}

}   // namespace NCloud::NBlockStore::NServer
