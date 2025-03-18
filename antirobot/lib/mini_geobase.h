#pragma once

#include <antirobot/idl/factors.pb.h>
#include <antirobot/lib/ip_interval.h>
#include <antirobot/lib/ip_hash.h>

#include <util/generic/noncopyable.h>
#include <util/stream/input.h>
#include <util/string/printf.h>

namespace NAntiRobot {

class TMiniGeobase : TMoveOnly {
public:
    enum class EIpType : ui32 {
        Mikrotik /* "mikrotik" */,
        Squid    /* "squid" */,
        Ddoser   /* "ddoser" */,
        Count
    };

    class TMask {
        ui64 Mask;
    public:
        explicit TMask(ui64 mask = 0) : Mask(mask) {
        }

        bool IsMikrotik() const {
            return Mask & (1ull << static_cast<ui32>(EIpType::Mikrotik));
        }

        bool IsSquid() const {
            return Mask & (1ull << static_cast<ui32>(EIpType::Squid));
        }

        bool IsDdoser() const {
            return Mask & (1ull << static_cast<ui32>(EIpType::Ddoser));
        }

        ui64 AsUint64() const {
            return Mask;
        }
    };

private:
    static constexpr size_t V4mask = 32;
    static constexpr size_t V6mask = 64;

    TIpHashMap<TMask, V4mask, V6mask> Map;

public:
    TMiniGeobase() = default;

    TMask GetValue(const TAddr& addr) const {
        const auto res = Map.Find(addr);
        return res.GetOrElse(TMask());
    }

    void Load(IInputStream& in) {
        NAntiRobot::NFeaturesProto::TFixed64Record record;
        NAntiRobot::NFeaturesProto::THeader header;
        NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);

        Y_ENSURE(NProtoBuf::io::ParseFromZeroCopyStreamSeq(&header, &adaptor) && header.GetHeader() == "TFixed64Record",
                "LoadError TMiniGeobase invalid header");
        size_t num = header.GetNum();

        while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&record, &adaptor)) {
            const auto key = record.GetKey();
            const TAddr addr(key);
            TString keyWithSuffix = key + Sprintf("/%lu", addr.IsIp6() ? V6mask : V4mask);
            const auto ipInterval = TIpInterval::Parse(keyWithSuffix);
            Map.Insert({ipInterval, TMask(record.GetValue())});
            --num;
        }
        Y_ENSURE(num == 0, "LoadError TMiniGeobase num mismatch");

        Map.Finish();
    }

private:
    friend void In<TMiniGeobase>(IInputStream& in, TMiniGeobase& dict);
};

}
