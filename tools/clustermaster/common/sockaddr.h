#pragma once

#include "log.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/network/socket.h>
#include <util/system/env.h>

#include <stdlib.h>
#include <utility>

class TSocketAddress {
private:
    typedef std::pair<TString, ui16> THostPort;
    typedef THashMap<THostPort, THostPort> TForwardMap;

private:
    const TNetworkAddress NetworkAddress;
    static const TForwardMap ForwardMap; // FORWARD_MAP format => fromHostname:fromPort|toHostname:toPort

private:
    static TStringBuf GetForwardName() {
        return GetEnv("FORWARD_MAP");
    }

    THostPort Resolve(const TString& host, ui16 port = 0) const {
        auto it = ForwardMap.find(std::make_pair(host, port));
        if (it != ForwardMap.end())
            return it->second;
        return std::make_pair(host, port);
    }

public:
    TSocketAddress(const TString& host, ui16 port)
        : NetworkAddress(Resolve(host).first, Resolve(host, port).second) {
        DEBUGLOG("Address has been resolved: " << host << ":" << port << " to " << Resolve(host).first << ":" << Resolve(host, port).second);
    }

    TSocketAddress(const TString& host, ui16 port, int flags)
        : NetworkAddress(Resolve(host).first, Resolve(host, port).second, flags) {
        DEBUGLOG("Address has been resolved: " << host << ":" << port << " to " << Resolve(host).first << ":" << Resolve(host, port).second << " flags=" << flags);
    }

    const TNetworkAddress& GetNetworkAddress() const {
        return NetworkAddress;
    }

    static TForwardMap ParseForwardMap(TStringBuf fwd);
};
