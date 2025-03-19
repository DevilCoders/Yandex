#pragma once

#include <util/system/types.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

/*
 *  The following two funcs recalculate maxIops and maxBandwidth so that
 *  we get the aforementioned maxIops IOPS and maxBandwidth bytes/sec
 *  for requestSize=4KiB and requestSize=4MiB respectively
 */

ui32 CalculateThrottlerC1(double maxIops, double maxBandwidth);
ui32 CalculateThrottlerC2(double maxIops, double maxBandwidth);

}   // namespace NCloud::NBlockStore
