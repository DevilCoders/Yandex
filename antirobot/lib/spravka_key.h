#pragma once

#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/generic/string.h>
#include <util/stream/input.h>

namespace NAntiRobot {

class TSpravkaKey {
public:
    using TInstanceAccessor = NThreading::TRcuAccessor<TSpravkaKey>;

    TSpravkaKey() = default;
    explicit TSpravkaKey(IInputStream& key);
    explicit TSpravkaKey(const TString& key);

    static TInstanceAccessor::TReference Instance() {
        return Singleton<TInstanceAccessor>()->Get();
    }

    static void SetInstance(TSpravkaKey instance) {
        Singleton<TInstanceAccessor>()->Set(std::move(instance));
    }

    TStringBuf Get() const {
        Y_ASSERT(!Key.empty());

        return Key;
    }

private:
    TString Key;
};

} // namespace NAntiRobot
