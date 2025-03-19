#pragma once

#include "private.h"

#include <cloud/storage/core/libs/common/public.h>

#include <cloud/blockstore/libs/rdma/public.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

IRunnablePtr CreateTestTarget(
    TOptionsPtr options,
    ITaskQueuePtr taskQueue,
    IStoragePtr storage,
    NRdma::IServerPtr server);

}   // namespace NCloud::NBlockStore
