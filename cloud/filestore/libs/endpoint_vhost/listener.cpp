#include "listener.h"

#include <cloud/filestore/libs/client/config.h>
#include <cloud/filestore/libs/client/session.h>
#include <cloud/filestore/libs/endpoint/listener.h>
#include <cloud/filestore/libs/fuse/config.h>
#include <cloud/filestore/libs/fuse/driver.h>
#include <cloud/filestore/libs/fuse/fs.h>
#include <cloud/filestore/libs/service/filestore.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

namespace NCloud::NFileStore::NVhost {

using namespace NThreading;
using namespace NCloud::NFileStore::NClient;
using namespace NCloud::NFileStore::NFuse;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TEndpoint final
    : public IEndpoint
{
private:
    const IFileSystemDriverPtr Driver;

public:
    TEndpoint(IFileSystemDriverPtr driver)
        : Driver(std::move(driver))
    {}

    TFuture<NProto::TError> StartAsync() override
    {
        return Driver->StartAsync();
    }

    TFuture<void> StopAsync() override
    {
        return Driver->StopAsync().IgnoreResult();
    }

    TFuture<void> SuspendAsync() override
    {
        return Driver->SuspendAsync().IgnoreResult();
    }

    TVector<TIncompleteRequest> GetIncompleteRequests() override
    {
        return Driver->GetIncompleteRequests();
    }
};

////////////////////////////////////////////////////////////////////////////////

class TEndpointListener final
    : public IEndpointListener
{
private:
    const ILoggingServicePtr Logging;
    const ITimerPtr Timer;
    const ISchedulerPtr Scheduler;
    const IRequestStatsRegistryPtr StatsRegistry;
    const IFileStoreEndpointsPtr FileStoreEndpoints;

    TLog Log;

    IFileSystemFactoryPtr FileSystemFactory;

public:
    TEndpointListener(
            ILoggingServicePtr logging,
            ITimerPtr timer,
            ISchedulerPtr scheduler,
            IRequestStatsRegistryPtr statsRegistry,
            IFileStoreEndpointsPtr filestoreEndpoints)
        : Logging(std::move(logging))
        , Timer(std::move(timer))
        , Scheduler(std::move(scheduler))
        , StatsRegistry(std::move(statsRegistry))
        , FileStoreEndpoints(std::move(filestoreEndpoints))
    {
        Log = Logging->CreateLog("VHOST");

        FileSystemFactory = CreateFuseFileSystemFactory(
            Logging,
            Scheduler,
            Timer);

    }

    IEndpointPtr CreateEndpoint(const NProto::TEndpointConfig& config) override
    {
        const auto& serviceEndpoint = config.GetServiceEndpoint();

        auto filestore = FileStoreEndpoints->GetEndpoint(serviceEndpoint);
        if (!filestore) {
            ythrow TServiceError(E_ARGUMENT)
                << "invalid service endpoint " << serviceEndpoint.Quote();
        }

        NProto::TSessionConfig sessionConfig;
        sessionConfig.SetFileSystemId(config.GetFileSystemId());
        sessionConfig.SetClientId(config.GetClientId());
        sessionConfig.SetSessionPingTimeout(config.GetSessionPingTimeout());
        sessionConfig.SetSessionPingTimeout(config.GetSessionRetryTimeout());

        auto session = CreateSession(
            Logging,
            Timer,
            Scheduler,
            std::move(filestore),
            std::make_shared<TSessionConfig>(sessionConfig));

        NProto::TFuseConfig protoConfig;
        protoConfig.SetFileSystemId(config.GetFileSystemId());
        protoConfig.SetClientId(config.GetClientId());
        protoConfig.SetSocketPath(config.GetSocketPath());
        protoConfig.SetReadOnly(config.GetReadOnly());
        protoConfig.SetMountSeqNumber(config.GetMountSeqNumber());
        protoConfig.SetVhostQueuesCount(config.GetVhostQueuesCount());
        if (Log.FiltrationLevel() >= TLOG_DEBUG) {
            protoConfig.SetDebug(true);
        }

        auto fuseConfig = std::make_shared<TFuseConfig>(protoConfig);
        auto driver = CreateFileSystemDriver(
            fuseConfig,
            Logging,
            StatsRegistry,
            session,
            FileSystemFactory);

        return std::make_shared<TEndpoint>(std::move(driver));
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IEndpointListenerPtr CreateEndpointListener(
    ILoggingServicePtr logging,
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    IRequestStatsRegistryPtr statsRegistry,
    IFileStoreEndpointsPtr filestoreEndpoints)
{
    return std::make_shared<TEndpointListener>(
        std::move(logging),
        std::move(timer),
        std::move(scheduler),
        std::move(statsRegistry),
        std::move(filestoreEndpoints));
}

}   // namespace NCloud::NFileStore::NVhost
