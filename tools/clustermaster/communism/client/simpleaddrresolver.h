#pragma once

#include <util/generic/string.h>
#include <util/network/socket.h>

class TSimpleAddressResolver {
public:
    TNetworkAddress operator() (const TString& host, TIpPort port) {
        return TNetworkAddress(host, port);
    }
};
