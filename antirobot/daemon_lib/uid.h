#pragma once

#include <antirobot/lib/addr.h>
#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/fuid.h>

#include <yweb/webdaemons/icookiedaemon/icookie_lib/utils/parse_icookie.h>

#include <library/cpp/lcookie/lcookie.h>

#include <util/digest/numeric.h>
#include <util/generic/strbuf.h>

#include <cstring>

namespace NAntiRobot {
    namespace NCacheSyncProto {
        class TUid;
    }

    class TAddr;
    class TSpravka;

#pragma pack(1)
    struct Y_PACKED TUid {
        enum ENameSpace {
            UNK = 0,
            IP = 1,
            SPRAVKA = 2,
            AGGR = 5,
            FUID = 6,
            LCOOKIE = 7,
            IP6 = 8,
            AGGR6 = 9,
            ICOOKIE = 10,
            PREVIEW = 11,
            AGGR_STR = 12,
            Count
        };

        static constexpr size_t Ip6Bits = 64;
        static const size_t NumOfIpAggregationLevels = 3;

        union {
            ui64 Id;
            struct {
                ui64 IdLo;
                ui64 IdHi;
            };
            char Data[16];
        };

        ui16 Ns;
        ui16 AggrLevel;

        inline TUid() {
            Zero(*this);
        }

        inline TUid(ENameSpace ns, ui64 id, ui16 aggrLevel = 0)
            : IdLo(id)
            , IdHi(0)
            , Ns(ns)
            , AggrLevel(aggrLevel)
        {
        }

        explicit TUid(const TAddr& addr);

        explicit TUid(const NCacheSyncProto::TUid& pbUid);

        TUid(const TUid& that) {
            std::memcpy(Data, that.Data, sizeof(Data));
            Ns = that.Ns;
            AggrLevel = that.AggrLevel;
        }

        TUid& operator=(const TUid& that) {
            std::memcpy(Data, that.Data, sizeof(Data));
            Ns = that.Ns;
            AggrLevel = that.AggrLevel;
            return *this;
        }

        static TUid Max() {
            TUid uid;
            uid.IdLo = ::Max<ui64>();
            uid.IdHi = ::Max<ui64>();
            uid.Ns = ::Max<ui16>();
            uid.AggrLevel = ::Max<ui16>();

            return uid;
        }

        inline bool operator==(const TUid& rhs) const {
            return !std::memcmp(&Data, &rhs.Data, sizeof(Data)) &&
                std::tie(Ns, AggrLevel) == std::tie(rhs.Ns, rhs.AggrLevel);
        }

        inline bool operator<(const TUid& rhs) const {
            return std::memcmp(&Data, &rhs.Data, sizeof(Data)) < 0 &&
                std::tie(Ns, AggrLevel) < std::tie(rhs.Ns, rhs.AggrLevel);
        }

        inline size_t Hash() const noexcept {
            return CombineHashes<size_t>(CombineHashes<size_t>(CombineHashes<size_t>((size_t)IdLo, (size_t)IdHi), (size_t)Ns), (size_t)AggrLevel);
        }

        inline bool Trusted() const noexcept {
            switch (Ns) {
                case SPRAVKA:
                case FUID:
                case LCOOKIE:
                case ICOOKIE:
                    return true;
                default:
                    return false;
            }
        }

        inline bool IpBased() const {
            return Ns == IP || Ns == IP6;
        }

        // Can be easily obtained
        inline bool Farmable() const {
            return Ns != SPRAVKA && Ns != LCOOKIE;
        }

        ENameSpace GetNameSpace() const {
            return static_cast<ENameSpace>(Ns);
        }

        int GetAggregationLevel() const {
            return Ns == AGGR || Ns == AGGR6 || Ns == AGGR_STR ? AggrLevel : -1;
        }

        TAddr ToAddr() const;

        static inline TUid FromAddr(const TAddr& addr) {
            return TUid(addr);
        }

        static TUid FromSubnet(const TAddr& addr, size_t subnetSizeBits = Ip6Bits);

        static TUid FromAddrOrSubnet(const TAddr& addr, size_t subnetSizeBits = Ip6Bits);

        static TUid FromAddrOrSubnetPreview(const TAddr& addr, ui16 type, size_t subnetSizeBits = Ip6Bits);

        static TUid FromJa3Aggregation(TStringBuf ja3);

        static TUid FromIpAggregation(const TAddr& addr, size_t level);

        static TUid FromFlashCookie(const TFlashCookie& cookie) {
            return TUid(FUID, cookie.Id());
        }

        static TUid FromSpravka(const TSpravka& s) noexcept;

        static TUid FromLCookie(const NLCookie::TLCookie& l) noexcept {
            return TUid(LCOOKIE, l.Uid);
        }

        static TUid FromICookie(const NIcookie::TIcookieDataWithRaw& i) noexcept {
            return TUid(ICOOKIE, i.Raw);
        }

        void SerializeTo(NCacheSyncProto::TUid* pbUid) const;
    };
#pragma pack()

    struct TUidHash {
        inline size_t operator()(const TUid& id) const noexcept {
            return id.Hash();
        }
    };
} // namespace NAntiRobot


inline TStringBuf ToString(NAntiRobot::TUid::ENameSpace ns) {
    using namespace NAntiRobot;

    switch (ns) {
    case TUid::UNK:
        return "0_unknown"_sb;
    case TUid::IP:
        return "1_ip"_sb;
    case TUid::SPRAVKA:
        return "2_spravka"_sb;
    case TUid::AGGR:
        return "5_aggr"_sb;
    case TUid::FUID:
        return "6_fuid"_sb;
    case TUid::LCOOKIE:
        return "7_lcookie"_sb;
    case TUid::IP6:
        return "8_ip6"_sb;
    case TUid::AGGR6:
        return "9_aggr6"_sb;
    case TUid::ICOOKIE:
        return "10_icookie"_sb;
    case TUid::PREVIEW:
        return "11_preview"_sb;
    case TUid::AGGR_STR:
        return "12_aggr_str"_sb;
    case TUid::Count:
        return "all"_sb;
    }

    return "invalid"_sb;
}


template <>
struct THash<NAntiRobot::TUid> {
    size_t operator()(const NAntiRobot::TUid& uid) const {
        return uid.Hash();
    }
};


Y_DECLARE_PODTYPE(NAntiRobot::TUid);
