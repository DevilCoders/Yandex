#pragma once

#include <antirobot/lib/addr.h>
#include <antirobot/lib/ip_map.h>

namespace NAntiRobot {

size_t ChooseInstance(const TAddr& addr, size_t attempt, size_t instanceCount, const TIpRangeMap<size_t>& customHashingMap);

}
