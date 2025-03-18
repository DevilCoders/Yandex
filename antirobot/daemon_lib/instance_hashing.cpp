#include "instance_hashing.h"

#include "config_global.h"

#include <util/digest/numeric.h>

namespace NAntiRobot {

size_t ChooseInstance(const TAddr& addr, size_t attempt, size_t instanceCount, const TIpRangeMap<size_t>& customHashingMap) {
    size_t subnetSize = addr.IsIp4() ? ANTIROBOT_DAEMON_CONFIG.IpV4SubnetBitsSizeForHashing
                                     : ANTIROBOT_DAEMON_CONFIG.IpV6SubnetBitsSizeForHashing;

    if (auto customSubnetSize = customHashingMap.Find(addr); Y_UNLIKELY(customSubnetSize.Defined())) {
        subnetSize = customSubnetSize.GetRef();
    }

    const TAddr subnet = addr.GetSubnet(subnetSize);
    return CombineHashes<size_t>(attempt, subnet.Hash()) % instanceCount;
}

}
