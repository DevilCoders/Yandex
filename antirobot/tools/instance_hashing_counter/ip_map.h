#pragma once

#include <library/cpp/ipv6_address/ipv6_address.h>

#include <util/generic/maybe.h>

namespace {
    struct TMyIpRange {
        TIpv6Address Start;
        TIpv6Address End;

        bool operator<(const TMyIpRange& other) const {
            return std::forward_as_tuple(Start, End) < std::forward_as_tuple(other.Start, other.End);
        }

        bool operator==(const TMyIpRange& other) const {
            return std::forward_as_tuple(Start, End) == std::forward_as_tuple(other.Start, other.End);
        }

        TString ToString() const {
            return Start.ToString() + " - " + End.ToString();
        }

        Y_SAVELOAD_DEFINE(Start, End);
    };
}

TMyIpRange ParseNetwork(TStringBuf network) {
    TStringBuf addressAsString, maskSizeAsString;
    network.Split('/', addressAsString, maskSizeAsString);

    bool ok;
    TIpv6Address address = TIpv6Address::FromString(TString{addressAsString}, ok);
    Y_ENSURE(ok, "Cannot parse IP address " << addressAsString << " of network " << network);

    int maskSize = FromString<int>(maskSizeAsString);
    Y_ENSURE(0 <= maskSize, "Expected non-negative number as a network mask size, but got "
                                << maskSize << "; network: " << network);

    TIpv6Address start, end;
    auto addressAsNumber = static_cast<ui128>(address);

    if (address.Type() == TIpv6Address::TIpType::Ipv4) {
        Y_ENSURE(maskSize <= 32, "IpV4 network mask size should not be greater than 32; mask size: "
                                     << maskSize << "; network: " << network);

        auto mask = ui128{ui64{1ULL << (32 - maskSize)} - 1};

        start = TIpv6Address(addressAsNumber & ~mask, TIpv6Address::TIpType::Ipv4);
        end = TIpv6Address(addressAsNumber | mask, TIpv6Address::TIpType::Ipv4);
    } else {
        Y_ENSURE(maskSize <= 128, "IpV6 network mask size should not be greater than 128; mask size: "
                                      << maskSize << "; network: " << network);

        const int shift = 128 - maskSize;
        const ui128 shifted = (shift < 128) ? (ui128{1} << shift) : 0;
        auto mask = ui128(shifted - 1);

        start = TIpv6Address(addressAsNumber & ~mask, TIpv6Address::TIpType::Ipv6);
        end = TIpv6Address(addressAsNumber | mask, TIpv6Address::TIpType::Ipv6);
    }

    return {start, end};
}

template <class TData>
class TIpRangeMap {
public:
    void Insert(const std::pair<TMyIpRange, TData>& item) {
        Y_ENSURE(!IpMap.contains(item.first) || IpMap[item.first] == item.second,
                 "Map already contains " << item.first.ToString() << " with another value. ");

        IpMap.insert(item);
    }

    TMaybe<TData> Find(const TIpv6Address& address) const {
        if (IpMap.empty()) {
            return Nothing();
        }

        auto it = IpMap.upper_bound(TMyIpRange{address, address});
        if (it == IpMap.begin() && it->first.Start != address) {
            return Nothing();
        }

        if (it == IpMap.end() || it->first.Start != address) {
            it--;
        }

        if (it->first.Start > address || address > it->first.End) {
            return Nothing();
        }

        return it->second;
    }

    void EnsureNoIntersections() const {
        if (IpMap.size() < 2) {
            return;
        }

        for (auto previous = IpMap.cbegin(), current = next(previous);
             current != IpMap.cend();
             previous++, current++) {
            Y_ENSURE(previous->first.End < current->first.Start,
                     "Found intersection "
                         << previous->first.Start << '-' << previous->first.End << " and "
                         << current->first.Start << '-' << current->first.End);
        }
    }

    void Clear() {
        IpMap.clear();
    }

    Y_SAVELOAD_DEFINE(IpMap);

private:
    TMap<TMyIpRange, TData> IpMap;
};
