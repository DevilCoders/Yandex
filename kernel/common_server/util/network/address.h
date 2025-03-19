#pragma once

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/network/socket.h>
#include <util/string/cast.h>
#include <util/system/defaults.h>

namespace NDns {
    struct TResolvedHost;
}

namespace NUtil {

TMaybe<const NDns::TResolvedHost*> Resolve(const TString& host, ui16 port) noexcept;

inline bool CanBeResolved(const TString& host, ui16 port) noexcept {
    return Resolve(host, port).Defined();
}

struct TComparableAddress {
    TString Host;
    ui16 Port;
    inline TComparableAddress(const TString& host, ui16 port)
        : Host(host)
        , Port(port)
    {
    }

    inline bool operator < (const TComparableAddress& rhs) const
    {
        return Host < rhs.Host || (Host == rhs.Host && Port < rhs.Port);
    }

    inline operator TNetworkAddress () const
    {
        return TNetworkAddress(Host, Port);
    }

    inline TString ToString() const
    {
        return Host + ':' + ::ToString(Port);
    }

    inline bool CanBeResolved() const {
        return NUtil::CanBeResolved(Host, Port);
    }
};

}

using TComparableAddress = NUtil::TComparableAddress;
