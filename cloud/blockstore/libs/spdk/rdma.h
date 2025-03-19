#pragma once

#include "public.h"

#include <cloud/blockstore/libs/rdma/rdma.h>

namespace NCloud::NBlockStore::NSpdk {

////////////////////////////////////////////////////////////////////////////////

NRdma::TRdmaHandler RdmaHandler();

}   // namespace NCloud::NBlockStore::NSpdk
