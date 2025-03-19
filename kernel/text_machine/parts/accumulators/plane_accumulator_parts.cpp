#include "plane_accumulator_parts.h"

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(PlaneAccumulator) {
        Y_CONST_FUNCTION ui32 CalcDist(ui32 leftPosition, ui32 rightPosition) {
            if (rightPosition <= leftPosition) {
                return 0;
            }
            return rightPosition - leftPosition - 1;
        }

        Y_CONST_FUNCTION float CalcStdProximity(ui32 dist) {
            if (0 == dist) {
                return 1.0f;
            }
            if (dist >= 64) {
                return 0.0f;
            }
            return 1.0f / float((dist + 1) * (dist + 1));
        }

        Y_CONST_FUNCTION float CalcStdAttenuation(ui32 position) {
            return 50.0f / float(position + 50);
        }

        Y_CONST_FUNCTION float CalcProximity(size_t proximityId, ui32 dist) {
            float distInv = 1.0 / ((float)dist + 1.0f);
            if (proximityId == 0) {
                return distInv;
            } else if (proximityId == 1) {
                return distInv * distInv;
            }
            float distInvSqr = distInv * distInv;
            return distInvSqr * distInvSqr * distInv;
        }

    }; //MACHINE_PARTS(PlaneAccumulator)
}; //NCore
}; //NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
