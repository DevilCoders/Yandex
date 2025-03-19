#pragma once

#include "public.h"

#include <cloud/blockstore/libs/common/public.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/service/service.h>

#include <cloud/storage/core/libs/diagnostics/incomplete_requests.h>

namespace NCloud::NBlockStore::NClient {

////////////////////////////////////////////////////////////////////////////////

struct IMetricClient
    : public IBlockStore
    , public IIncompleteRequestProvider
{};

////////////////////////////////////////////////////////////////////////////////

IMetricClientPtr CreateMetricClient(
    IBlockStorePtr client,
    const TLog& log,
    IServerStatsPtr serverStats);

}   // namespace NCloud::NBlockStore::NClient
