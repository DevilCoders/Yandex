#pragma once

#include "private.h"

#include <cloud/filestore/config/vhost.pb.h>
#include <cloud/filestore/libs/client/public.h>
#include <cloud/filestore/libs/diagnostics/public.h>
#include <cloud/filestore/libs/endpoint/public.h>
#include <cloud/filestore/libs/endpoint_vhost/public.h>
#include <cloud/filestore/libs/daemon/bootstrap.h>
#include <cloud/filestore/libs/server/public.h>
#include <cloud/filestore/libs/service/public.h>

#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/public.h>
#include <cloud/storage/core/libs/keyring/public.h>

#include <library/cpp/actors/util/should_continue.h>

namespace NCloud::NFileStore::NServer {

////////////////////////////////////////////////////////////////////////////////

class TVhostServerBootstrap final
    : public NDaemon::TBootstrap
{
private:
    TOptionsPtr Options;
    TServerConfigPtr ServerConfig;

    NVhost::TVhostServiceConfigPtr VhostServiceConfig;
    IFileStoreEndpointsPtr FileStoreEndpoints;
    IEndpointListenerPtr EndpointListener;
    IEndpointStoragePtr EndpointStorage;
    IEndpointManagerPtr EndpointManager;

public:
    TVhostServerBootstrap(TOptionsPtr options);
    ~TVhostServerBootstrap();

private:
    void StartComponents() override;
    void StopComponents() override;
    void InitLWTrace() override;
    IServerPtr CreateServer() override;
    TVector<IIncompleteRequestProviderPtr> GetIncompleteRequestProviders() override;

private:
    void InitConfig();
    void InitEndpoints();
    void InitNullEndpoints();
    void RestoreKeyringEndpoints();
};

}   // namespace NCloud::NFileStore::NServer
