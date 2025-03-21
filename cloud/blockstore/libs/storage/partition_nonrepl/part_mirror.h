#pragma once

#include "public.h"

#include "config.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/kikimr/public.h>
#include <cloud/blockstore/libs/rdma/public.h>
#include <cloud/blockstore/libs/storage/core/public.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

NActors::IActorPtr CreateMirrorPartition(
    TStorageConfigPtr config,
    IProfileLogPtr profileLog,
    IBlockDigestGeneratorPtr digestGenerator,
    TString rwClientId,
    TNonreplicatedPartitionConfigPtr partConfig,
    TVector<TDevices> replicas,
    NRdma::IClientPtr rdmaClient);

}   // namespace NCloud::NBlockStore::NStorage
