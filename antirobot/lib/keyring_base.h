#pragma once

#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/stream/str.h>


namespace NAntiRobot {


template<typename TKey, typename TSelf>
class TKeyRingBase {
public:
    using TInstanceAccessor = NThreading::TRcuAccessor<TSelf>;

    TKeyRingBase() = default;

    explicit TKeyRingBase(IInputStream& keys) {
        LoadKeys(keys);
    }

    explicit TKeyRingBase(const TString& keys) {
        TStringStream in;
        in << keys;
        LoadKeys(in);
    }

    static typename TInstanceAccessor::TReference Instance() {
        return Singleton<TInstanceAccessor>()->Get();
    }

    static void SetInstance(TSelf instance) {
        Singleton<TInstanceAccessor>()->Set(std::move(instance));
    }

private:
    void Add(const TKey& key) {
        Keys.push_back(key);
    }

    void LoadKeys(IInputStream& keys) {
        TString line;
        while (keys.ReadLine(line) > 0) {
            Add(TKey(line));
        }
    }

protected:
    TVector<TKey> Keys;
};


} // namespace NAntiRobot
