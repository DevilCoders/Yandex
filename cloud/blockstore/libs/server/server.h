#pragma once

#include "public.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/service/public.h>

#include <cloud/storage/core/libs/common/startable.h>
#include <cloud/storage/core/libs/diagnostics/incomplete_requests.h>

namespace NCloud::NBlockStore::NServer {

////////////////////////////////////////////////////////////////////////////////

struct IServer
    : public IStartable
    , public IIncompleteRequestProvider
{
    virtual IClientAcceptorPtr GetClientAcceptor() = 0;
};

////////////////////////////////////////////////////////////////////////////////

IServerPtr CreateServer(
    TServerAppConfigPtr config,
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IBlockStorePtr service);

}   // namespace NCloud::NBlockStore::NServer
