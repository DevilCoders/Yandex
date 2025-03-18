#include "host_addr.h"

#include <library/cpp/ipv6_address/ipv6_address.h>

#include <util/stream/output.h>
#include <util/string/cast.h>

namespace NAntiRobot {

  std::tuple<TNetworkAddress, TString, ui16> CreateNetworkAddress(const TString& str) {
    bool ok;
    auto result = ParseHostAndMayBePortFromString(str, 80, ok);

    if (!ok) {
        ythrow yexception() << "Host string parsing error: " << str;
    }

    auto hostPort = std::get<0>(result);
    TString host = std::get<1>(result);
    ui16 port = std::get<2>(result);

    auto addBrackets = [](const TString& s) { return s.find(':') != TString::npos ? "[" + s + "]" : s; };
    if (host.empty()) {
        host = hostPort.Ip.ToString(ok);
        return { TNetworkAddress(host, hostPort.Port), addBrackets(host), hostPort.Port };
    }

    return { TNetworkAddress(host, port), addBrackets(host), port };
}

THostAddr CreateHostAddr(const TString& str) {
    auto result = CreateNetworkAddress(str);
    auto addr = std::get<0>(result);
    auto host = std::get<1>(result);
    auto port = std::get<2>(result);
    return THostAddr(addr, host, port, host);
}

THostAddr::THostAddr()
    : TNetworkAddress(0)
{
}

  THostAddr::THostAddr(const TNetworkAddress& addr, const TString& hostOrIp, ui16 port, const TString& hostName)
    : TNetworkAddress(addr)
    , HostOrIp(hostOrIp)
    , Port(port)
    , HostName(hostName)
{
}

IOutputStream& operator << (IOutputStream& os, const THostAddr& addr) {
    return os << addr.HostName;
}

} /* namespace NAntiRobot */

template<>
NAntiRobot::THostAddr FromStringImpl<NAntiRobot::THostAddr>(const char* data, size_t len) {
    return NAntiRobot::CreateHostAddr(TString(data, len));
}

