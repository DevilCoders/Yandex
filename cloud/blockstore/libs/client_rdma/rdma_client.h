#pragma once

#include "public.h"

#include <cloud/blockstore/libs/rdma/public.h>
#include <cloud/blockstore/libs/service/public.h>

#include <cloud/storage/core/libs/diagnostics/public.h>

#include <util/generic/string.h>

namespace NCloud::NBlockStore::NClient {

////////////////////////////////////////////////////////////////////////////////

struct TRdmaEndpointConfig
{
    TString Address;
    ui32 Port;
};

////////////////////////////////////////////////////////////////////////////////

IBlockStorePtr CreateRdmaEndpointClient(
    ILoggingServicePtr logging,
    NRdma::IClientPtr client,
    IBlockStorePtr volumeClient,
    const TRdmaEndpointConfig& config);

}   // namespace NCloud::NBlockStore::NClient
