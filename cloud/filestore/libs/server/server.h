#pragma once

#include "public.h"

#include <cloud/filestore/libs/diagnostics/public.h>

#include <cloud/filestore/libs/service/public.h>

#include <cloud/storage/core/libs/common/startable.h>
#include <cloud/storage/core/libs/diagnostics/incomplete_requests.h>
#include <cloud/storage/core/libs/diagnostics/public.h>

namespace NCloud::NFileStore::NServer {

////////////////////////////////////////////////////////////////////////////////

struct IServer
    : public IStartable
    , public IIncompleteRequestProvider
{
};

////////////////////////////////////////////////////////////////////////////////

IServerPtr CreateServer(
    TServerConfigPtr config,
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IProfileLogPtr profileLog,
    IFileStoreServicePtr service);

IServerPtr CreateServer(
    TServerConfigPtr config,
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IProfileLogPtr profileLog,
    IEndpointManagerPtr service);

}   // namespace NCloud::NFileStore::NServer
