#pragma once
#include <util/generic/string.h>
#include <util/network/socket.h>

#include <tuple>

namespace NAntiRobot {

struct THostAddr : public TNetworkAddress {
    TString HostOrIp;
    ui16 Port;
    TString HostName;

    THostAddr();
    THostAddr(const TNetworkAddress& addr, const TString& hostOrIp, ui16 port, const TString& hostName);
};

std::tuple<TNetworkAddress, TString, ui16> CreateNetworkAddress(const TString& addr);
THostAddr CreateHostAddr(const TString& addr);
IOutputStream& operator << (IOutputStream& os, const THostAddr& addr);

} /* namespace NAntiRobot */
