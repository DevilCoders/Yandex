#pragma once

#include "ip_map.h"

#include <antirobot/tools/instance_hashing_counter/proto/l7_access_log_row.pb.h>
#include <antirobot/tools/instance_hashing_counter/proto/daemon_log_row.pb.h>

#include <antirobot/daemon_lib/config_global.h>
#include <antirobot/daemon_lib/instance_hashing.h>

#include <library/cpp/ipv6_address/ipv6_address.h>
#include <library/cpp/yson/node/node.h>

namespace {
    using NAntiRobot::TAddr;
    using NAntiRobot::TAntirobotConfig;
    using NAntiRobot::TAntirobotDaemonConfig;

    // TODO: Get from Nanny-API
    const TString Instances[] = {
#include "hosts_vla.inc"
    };
    constexpr size_t InstancesCount = Y_ARRAY_SIZE(Instances);
} // anonymous namespace

TString GetInstanceName(size_t idx) {
    Y_ENSURE(idx < InstancesCount, "Instance index is out of bounds [0-" << InstancesCount << "]");

    return Instances[idx];
}

constexpr size_t GetInstanceCount() {
    return InstancesCount;
}

bool BadIpPort(const TString& ipPort) {
    return ipPort.StartsWith("127.") || ipPort.StartsWith("[2a02:6b8");
}

TAddr GetIpFromIpPort(const TString& ipPort) {
    TStringBuf ip, port;
    TStringBuf(ipPort).RSplit(':', ip, port);
    if (ip[0] != '[') {
        return TAddr(ip);
    }
    return TAddr(ip.Skip(1).Chop(1));
}

Y_DECLARE_UNUSED
TAddr GetIpFromWorkflow(const TString& workflow) {
    TStringBuf l, r;
    TStringBuf(workflow).Split("X-Forwarded-For-Y:", l, r);
    if (!r.Empty()) {
        r.Split("::>", l, r);
        if (l.size() < 40) {
            return TAddr(l);
        }
    }
    TStringBuf(workflow).Split("x-forwarded-for-y:", l, r);
    r.Split("::>", l, r);
    return TAddr(l);
}

bool ChangeSubnetSizeIfNeeded(const TAddr& addr, const TIpRangeMap<size_t>& customHashRules) {
    bool ok;
    auto addrAsIpv6 = TIpv6Address::FromString(addr.ToString(), ok);
    if (auto maybeNewSize = customHashRules.Find(addrAsIpv6); maybeNewSize.Defined()) {
        if (addr.IsIp4()) {
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.IpV4SubnetBitsSizeForHashing = maybeNewSize.GetRef();
        } else {
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.IpV6SubnetBitsSizeForHashing = maybeNewSize.GetRef();
        }
        return true;
    }
    return false;
}

template<typename TInputRow>
TMaybe<std::pair<size_t, TString>> TryGetIndexAndSubnetByRow(const TInputRow& row,
                                                             const TIpRangeMap<size_t>& customHashRules) {
    TAddr addr;
    if constexpr (std::is_same_v<TInputRow, TL7AccessLogRow>) {
        const auto& ipPort = row.GetIpPort();
        if (Y_UNLIKELY(BadIpPort(ipPort))) {
            addr = GetIpFromWorkflow(row.GetWorkflow());
        } else {
            addr = GetIpFromIpPort(ipPort);
        }
    } else if constexpr (std::is_same_v<TInputRow, TDaemonLogRow>) {
        addr = TAddr(row.GetSpravkaIp());
    } else {
        static_assert(TDependentFalse<TInputRow>);
    }

    if (!addr.Valid()) {
        return Nothing();
    }

    bool needToRestore = false;
    const size_t previousIpv4SubnetSize = ANTIROBOT_DAEMON_CONFIG.IpV4SubnetBitsSizeForHashing;
    const size_t previousIpv6SubnetSize = ANTIROBOT_DAEMON_CONFIG.IpV6SubnetBitsSizeForHashing;
    if (ChangeSubnetSizeIfNeeded(addr, customHashRules)) {
        needToRestore = true;
    }

    auto idx = ChooseInstance(addr, 0, InstancesCount, {});

    size_t subnetSize = addr.IsIp4() ? ANTIROBOT_DAEMON_CONFIG.IpV4SubnetBitsSizeForHashing
                                     : ANTIROBOT_DAEMON_CONFIG.IpV6SubnetBitsSizeForHashing;

    const TAddr subnet = addr.GetSubnet(subnetSize);

    if (needToRestore) {
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.IpV4SubnetBitsSizeForHashing = previousIpv4SubnetSize;
        ANTIROBOT_DAEMON_CONFIG_MUTABLE.IpV6SubnetBitsSizeForHashing = previousIpv6SubnetSize;
    }

    return std::make_pair(idx, subnet.ToString() + "/" + ToString(subnetSize));
}
