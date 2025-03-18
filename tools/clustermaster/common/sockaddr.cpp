#include "sockaddr.h"

#include <util/string/cast.h>

TSocketAddress::TForwardMap TSocketAddress::ParseForwardMap(TStringBuf fwd) {
    TForwardMap forwardMap;
    TStringBuf rec;

    while (fwd.NextTok(' ', rec)) {
        TStringBuf from, to;
        rec.Split('|', from, to);

        TStringBuf fromhost, fromport;
        from.Split(':', fromhost, fromport);

        TStringBuf tohost, toport;
        to.Split(':', tohost, toport);

        forwardMap.insert(TForwardMap::value_type(
            std::make_pair(TString(fromhost), 0),
            std::make_pair(TString(tohost), 0))
        );
        forwardMap.insert(TForwardMap::value_type(
            std::make_pair(TString(fromhost), FromString(fromport)),
            std::make_pair(TString(tohost), FromString(toport)))
        );

        DEBUGLOG("Address has been parsed: " << fromhost << "." << fromport << " " << tohost << "." << toport);
    }

    return forwardMap;
}

const TSocketAddress::TForwardMap TSocketAddress::ForwardMap = TSocketAddress::ParseForwardMap(TSocketAddress::GetForwardName());
