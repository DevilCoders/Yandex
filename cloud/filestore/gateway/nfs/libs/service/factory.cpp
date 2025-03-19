#include "factory.h"

#include "config.h"
#include "service.h"

#include <cloud/filestore/libs/client/client.h>
#include <cloud/filestore/libs/client/config.h>
#include <cloud/filestore/libs/client/durable.h>
#include <cloud/filestore/libs/client/session.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/stream/file.h>

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void ParseProtoTextFromFile(const TString& fileName, T& dst)
{
    TFileInput in(fileName);
    ParseFromTextFormat(in, dst);
}

////////////////////////////////////////////////////////////////////////////////

TFileStoreServiceFactory::TFileStoreServiceFactory(
        ITimerPtr timer,
        ISchedulerPtr scheduler,
        ILoggingServicePtr logging)
    : Timer(std::move(timer))
    , Scheduler(std::move(scheduler))
    , Logging(std::move(logging))
{
    Log = Logging->CreateLog("NFS");

    Init(this);
}

template <typename Method, typename... Args>
int TFileStoreServiceFactory::Call(Method&& m, yfs_service_factory* sf, Args&&... args) noexcept
{
    auto* pThis = static_cast<TFileStoreServiceFactory*>(sf);
    auto& Log = pThis->Log;
    try {
        return (pThis->*m)(std::forward<Args>(args)...);
    } catch (const TServiceError& e) {
        STORAGE_ERROR("unexpected error: "
            << FormatResultCode(e.GetCode()) << " " << e.GetMessage());
        return -ErrnoFromError(e.GetCode());
    } catch (...) {
        STORAGE_ERROR("unexpected error: " << CurrentExceptionMessage());
        return -EFAULT;
    }
}

void TFileStoreServiceFactory::Init(yfs_service_factory* sf)
{
#define CALL(m, svc, ...) \
    TFileStoreServiceFactory::Call(&TFileStoreServiceFactory::m, sf, __VA_ARGS__)

    sf->create = [] (
        yfs_service_factory* sf,
        const char* configPath,
        const char* filesystemId,
        const char* clientId,
        yfs_service** svc)
    {
        return CALL(Create, sf, configPath, filesystemId, clientId, svc);
    };
    sf->destroy = [] (yfs_service_factory* sf, yfs_service* svc) {
        return CALL(Destroy, sf, svc);
    };

#undef CALL
}

////////////////////////////////////////////////////////////////////////////////

int TFileStoreServiceFactory::Create(
    const TString& configPath,
    const TString& filesystemId,
    const TString& clientId,
    yfs_service** svc)
{
    STORAGE_DEBUG("Load client config from file " << configPath.Quote());

    NProto::TNfsGatewayConfig proto;
    ParseProtoTextFromFile(configPath, proto);

    auto serviceConfig = std::make_shared<TFileStoreServiceConfig>(proto);
    auto clientConfig = std::make_shared<NClient::TClientConfig>(proto.GetClientConfig());

    auto client = CreateFileStoreClient(clientConfig, Logging);
    client->Start();

    auto durableClient = CreateDurableClient(
        Logging,
        Timer,
        Scheduler,
        CreateRetryPolicy(clientConfig),
        client);

    NProto::TSessionConfig sessionConfig;
    sessionConfig.SetFileSystemId(filesystemId);
    sessionConfig.SetClientId(clientId);
    sessionConfig.SetSessionRetryTimeout(serviceConfig->GetSessionRetryTimeout().MilliSeconds());
    sessionConfig.SetSessionPingTimeout(serviceConfig->GetSessionPingTimeout().MilliSeconds());

    auto session = CreateSession(
        Logging,
        Timer,
        Scheduler,
        durableClient,
        std::make_shared<NClient::TSessionConfig>(sessionConfig));

    auto service = std::make_unique<TFileStoreService>(
        Logging,
        session,
        serviceConfig);

    service->Start();

    // service will be destroyed by calling Destroy method
    *svc = static_cast<yfs_service*>(service.release());
    return 0;
}

int TFileStoreServiceFactory::Destroy(yfs_service* svc)
{
    std::unique_ptr<TFileStoreService> service(static_cast<TFileStoreService*>(svc));
    service->Stop();

    return 0;
}

}   // namespace NCloud::NFileStore::NGateway
