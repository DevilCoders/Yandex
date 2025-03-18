#include "uid.h"

#include <antirobot/idl/cache_sync.pb.h>

#include <antirobot/lib/spravka.h>

#include <util/stream/format.h>
#include <util/stream/output.h>
#include <util/digest/city.h>

#include <array>

using namespace NAntiRobot;

static_assert(sizeof(TUid) == 20, "expect sizeof(TUid) == 20");

TUid::TUid(const TAddr& addr)
    : AggrLevel(0)
{
    switch(addr.GetFamily()) {
    case AF_INET: {
            Ns = IP;
            Id = addr.AsIp();
            IdHi = 0;
            break;
        }

    case AF_INET6: {
            Ns = IP6;
            TStringBuf ip6 = addr.AsIp6();
            std::memcpy(Data, ip6.data(), ip6.size());
            break;
        }

    default:
        ythrow yexception() << "Unsupported address family: " << addr.GetFamily();
    }
}

TUid::TUid(const NCacheSyncProto::TUid& pbUid) {
    IdLo = pbUid.GetIdLo();
    IdHi = pbUid.GetIdHi();
    Ns = pbUid.GetNamespace();
    AggrLevel = pbUid.GetAggrLevel();

    Y_ENSURE(Ns < TUid::ENameSpace::Count, "Bad uid namespace: " << Ns);
}

TAddr TUid::ToAddr() const {
    switch (Ns) {
    case IP:
        return TAddr::FromIp(ui32(Id));

    case IP6:
        return TAddr::FromIp6(TStringBuf(Data, Data + sizeof(Data)));

    default:
        ythrow TFromStringException() << "Wrong Uid namespace: " << Ns;
    }

}

TUid TUid::FromIpAggregation(const TAddr& addr, size_t level) {
    switch (addr.GetFamily()) {
    case AF_INET: {
        constexpr std::array<size_t, NumOfIpAggregationLevels> subnetSize = {32, 24, 16};

        return TUid(AGGR, addr.GetSubnet(subnetSize[level]).AsIp(), level);
    }
    case AF_INET6: {
        TUid res(AGGR6, 0, level);
        constexpr std::array<size_t, NumOfIpAggregationLevels> subnetSize = {64, 56, 48};
        auto subnet = addr.GetSubnet(subnetSize[level]);
        TStringBuf ip6 = subnet.AsIp6();
        std::memcpy(res.Data, ip6.data(), ip6.size());
        return res;
    }
    default:
        ythrow yexception() << "Unsupported address family: " << addr.GetFamily();
    }
}

TUid TUid::FromJa3Aggregation(TStringBuf ja3) {
    return TUid(AGGR_STR, CityHash64(ja3), NumOfIpAggregationLevels);
    // NOTE: если добавлять новый (скажем, ja4), то тип будет AGGR_STR, поменяется только level
}

TUid TUid::FromSpravka(const TSpravka& s) noexcept {
    return TUid(SPRAVKA, s.Uid);
}

TUid TUid::FromSubnet(const TAddr &addr, size_t subnetSizeBits) {
    return TUid(addr.GetSubnet(subnetSizeBits));
}

TUid TUid::FromAddrOrSubnet(const TAddr& addr, size_t subnetSizeBits) {
    return addr.IsIp4() ? FromAddr(addr) : FromSubnet(addr, subnetSizeBits);
}

TUid TUid::FromAddrOrSubnetPreview(const TAddr& addr, ui16 type, size_t subnetSizeBits) {
    static_assert(
        Ip6Bits <= 64,
        "PREVIEW uid encoding is not compatible with Ip6Bits above 64"
    );

    TUid uid = FromAddrOrSubnet(addr, subnetSizeBits);
    uid.Ns = PREVIEW;
    uid.IdHi ^= type << 1ull;

    if (addr.IsIp6()) {
        uid.IdHi ^= 1;
    }

    return uid;
}

void TUid::SerializeTo(NCacheSyncProto::TUid* pbUid) const {
    pbUid->SetIdLo(IdLo);
    pbUid->SetIdHi(IdHi);
    pbUid->SetNamespace(Ns);
    pbUid->SetAggrLevel(AggrLevel);
}

namespace {
ui64 readHexId(IInputStream& in) {
    char buf[sizeof(ui64) * 2 + 1] = {};
    size_t toRead = Y_ARRAY_SIZE(buf) - 1;
    if (in.Read(buf, toRead) != toRead) {
        ythrow TFromStringException() << "Failed to parse Uid's ID: " << buf;
    }
    return IntFromString<ui64, 16>(TString(buf));
}
}

template <>
TUid FromString(const TString& s) {
    TStringInput in(s);

    ui16 ns = FromString<ui16>(in.ReadTo('-'));
    ui16 aggrLevel = 0;
    ui64 id = 0;
    ui64 idHi = 0;

    switch (ns) {
    case TUid::UNK:
    case TUid::IP:
    case TUid::SPRAVKA:
    case TUid::ICOOKIE:
    case TUid::LCOOKIE:
        id = FromString<ui64>(in.ReadAll());
        break;
    case TUid::FUID: {
        TFlashCookie fuid;
        in >> fuid;
        id = fuid.Id();
        break;
    }
    case TUid::AGGR:
    case TUid::AGGR_STR:
        id = FromString<ui64>(in.ReadTo('-'));
        aggrLevel = FromString<ui16>(in.ReadAll());
        break;
    case TUid::IP6:
        id = readHexId(in);
        idHi = readHexId(in);
        if (in.Skip(1)) { // if there is some more data in the string we throw
            ythrow TFromStringException() << "Extra data in the string: " << in.ReadAll();
        }
        break;
    case TUid::AGGR6:
        {
            TString ids = in.ReadTo('-');
            TStringInput si(ids);
            id = readHexId(si);
            idHi = readHexId(si);
            aggrLevel = FromString<ui16>(in.ReadAll());
        }
        break;
    case TUid::PREVIEW:
        id = FromString<ui64>(in.ReadTo('-'));
        idHi = FromString<ui64>(in.ReadAll());
        break;
    default:
        ythrow TFromStringException() << "Wrong Uid namespace: " << ns;
    }

    TUid result;
    result.Ns = ns;
    if (result.Ns == TUid::IP6 || result.Ns == TUid::AGGR6) {
        result.Id = HostToInet(id);
        result.IdHi = HostToInet(idHi);
    } else if (result.Ns == TUid::PREVIEW) {
        result.Id = id;
        result.IdHi = idHi;
    } else {
        result.Id = id;
    }
    if (result.Ns == TUid::AGGR || result.Ns == TUid::AGGR6 || result.Ns == TUid::AGGR_STR) {
        result.AggrLevel = aggrLevel;
    }
    return result;
}

template <>
void In<TUid>(IInputStream& in, TUid& uid) {
    TString s;
    in >> s;
    uid = FromString<TUid>(s);
}

template <>
void Out<TUid::ENameSpace>(IOutputStream& out, TUid::ENameSpace ns) {
    out << (int)ns;
}

template <>
void Out<TUid>(IOutputStream& out, const TUid& uid) {
    if (uid.Ns == TUid::IP6 || uid.Ns == TUid::AGGR6) {
        out << uid.Ns << '-' << Hex(InetToHost(uid.Id), HF_FULL) << Hex(InetToHost(uid.IdHi), HF_FULL);
    } else if (uid.Ns == TUid::FUID) {
        out << uid.Ns << '-' << TFlashCookie::FromId(uid.Id);
    } else if (uid.Ns == TUid::PREVIEW) {
        out << uid.Ns << '-' << uid.Id << '-' << uid.IdHi;
    } else {
        out << uid.Ns << '-' << uid.Id;
    }

    if (uid.Ns == TUid::AGGR || uid.Ns == TUid::AGGR6 || uid.Ns == TUid::AGGR_STR) {
        out << '-' << uid.AggrLevel;
    }
}
