#include "throttler_formula.h"

#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

ui32 CalculateThrottlerC1(double maxIops, double maxBandwidth)
{
    if (maxBandwidth == 0) {
        return maxIops;
    }

   const auto denominator = Max(1_KB / maxIops - 4_MB / maxBandwidth, 0.);

   if (abs(denominator) < 1e-5) {
       // fallback for "special" params
       return maxIops;
   }

   return Max<ui32>((1_KB - 1) / denominator, 1);
}

ui32 CalculateThrottlerC2(double maxIops, double maxBandwidth)
{
   if (maxBandwidth == 0) {
       return 0;
   }

   const auto denominator = Max(4_MB / maxBandwidth - 1 / maxIops, 0.);

   if (abs(denominator) < 1e-5) {
       // fallback for "special" params
       return Max<ui32>();
   }

   return Max<ui32>(Min<ui64>((4_MB - 4_KB) / denominator, Max<ui32>()), 1);
}

}   // namespace NCloud::NBlockStore
