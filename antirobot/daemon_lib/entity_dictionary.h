#pragma once

#include "json_config.h"
#include "market_stats.h"

#include <antirobot/idl/factors.pb.h>
#include <antirobot/lib/ip_hash.h>

#include <google/protobuf/messagext.h>

#include <util/digest/city.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/string/printf.h>

#include <limits>

namespace NAntiRobot {

template <typename T>
struct TDictValueDefault {
    static constexpr T Value = T();
};

template <>
struct TDictValueDefault<float> {
    static constexpr float Value = std::numeric_limits<float>::quiet_NaN();
};

template<typename T>
class TEntityDict : TMoveOnly {
private:
    THashMap<TString, T> Map;

public:
    TEntityDict() = default;

    TEntityDict(std::initializer_list<std::pair<TStringBuf, T>> container)
        : Map(std::begin(container), std::end(container)) {
    }

    T GetValue(TStringBuf key) const {
        const auto it = Map.find(key);
        if (it == Map.end()) {
            return TDictValueDefault<T>::Value;
        }
        return it->second;
    }

private:
    friend void In<TEntityDict>(IInputStream&, TEntityDict&);
};

using TEntityDictionary = TEntityDict<float>;
using TMarketJwsStatsDictionary = TEntityDict<TMarketJwsStatesStats>;

template<typename T>
class THashDict : TMoveOnly {
private:
    THashMap<ui64, T> Map;

public:
    THashDict() = default;

    THashDict(std::initializer_list<std::pair<ui64, float>> container)
        : Map(std::begin(container), std::end(container)) {
    }

    T GetValue(TStringBuf key) const {
        const auto hash = CityHash64(key);
        const auto it = Map.find(hash);
        if (it == Map.end()) {
            return TDictValueDefault<T>::Value;
        }
        return it->second;
    }

    T GetValue(TStringBuf key, T defaultValue) const {
        const auto hash = CityHash64(key);
        const auto it = Map.find(hash);
        return it == Map.end() ? defaultValue : it->second;
    }

private:
    friend void In<THashDict>(IInputStream&, THashDict&);
};

using THashDictionary = THashDict<float>;
using TMarketStatsDictionary = THashDict<TMarketStats>;

template<size_t v4mask, size_t v6mask>
class TSubnetDictionary : TMoveOnly {
private:
    TIpHashMap<float, v4mask, v6mask> Map;

public:
    TSubnetDictionary() = default;

    TSubnetDictionary(std::initializer_list<std::pair<TString, float>> container) {
        for (const auto& [key, value] : container) {
            AddEntity(key, value);
        }
    }

    float GetValue(const TAddr& addr) const {
        const auto res = Map.Find(addr);
        return res.GetOrElse(std::numeric_limits<float>::quiet_NaN());
    }

    void Load(IInputStream& in) {
        NAntiRobot::NFeaturesProto::TFloatRecord record;
        NAntiRobot::NFeaturesProto::THeader header;
        NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);

        Y_ENSURE(NProtoBuf::io::ParseFromZeroCopyStreamSeq(&header, &adaptor) && header.GetHeader() == "TFloatRecord",
                "LoadError TSubnetDictionary invalid header");
        size_t num = header.GetNum();

        while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&record, &adaptor)) {
            AddEntity(record.GetKey(), record.GetValue());
            --num;
        }
        Y_ENSURE(num == 0, "LoadError TSubnetDictionary num mismatch");

        Finish();
    }

private:
    void AddEntity(const TString& key, float value) {
        const TAddr addr(key);
        TString keyWithSuffix = key + Sprintf("/%lu", addr.IsIp6() ? v6mask : v4mask);
        const auto ipInterval = TIpInterval::Parse(keyWithSuffix);
        Map.Insert({ipInterval, value});
    }

    void Finish() {
        Map.Finish();
    }

    friend void In<TSubnetDictionary>(IInputStream& in, TSubnetDictionary& dict);
};

} // namespace NAntiRobot
