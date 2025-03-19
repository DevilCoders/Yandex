#pragma once

#include <cloud/blockstore/libs/throttling/public.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

IThrottlerTrackerPtr CreateThrottlingServiceTracker();

}   // namespace NCloud::NBlockStore
