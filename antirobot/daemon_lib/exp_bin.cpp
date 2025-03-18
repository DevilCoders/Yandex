#include "exp_bin.h"

#include <util/stream/output.h>


template <>
void Out<NAntiRobot::EExpBin>(IOutputStream& out, NAntiRobot::EExpBin expBin) {
    out << static_cast<size_t>(EnumValue(expBin));
}
