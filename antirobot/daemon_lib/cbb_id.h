#pragma once

#include "exp_bin.h"

#include <antirobot/lib/tuple_ops.h>

#include <util/digest/numeric.h>
#include <util/generic/hash.h>

#include <tuple>


namespace NAntiRobot {


namespace NCacheSyncProto {
    class TCbbRule;
}


enum class TCbbGroupId : ui32 {};
enum class TCbbRuleId : ui32 {};

struct TCbbRuleKey : public TTupleComparable<TCbbRuleKey> {
    TCbbGroupId Group;
    TCbbRuleId Rule;

    TCbbRuleKey() = default;

    TCbbRuleKey(TCbbGroupId group, TCbbRuleId rule)
        : Group(group)
        , Rule(rule)
    {}

    explicit TCbbRuleKey(const NCacheSyncProto::TCbbRule& pbRule);

    void SerializeTo(NCacheSyncProto::TCbbRule* pbRule) const;

    std::tuple<const TCbbGroupId&, const TCbbRuleId&> AsTuple() const {
        return std::tie(Group, Rule);
    }
};


struct TBinnedCbbRuleKey {
    TCbbRuleKey Key;
    EExpBin Bin = EExpBin::Count;

    TBinnedCbbRuleKey(TCbbRuleKey key, EExpBin bin)
        : Key(key)
        , Bin(bin)
    {}

    TBinnedCbbRuleKey(TCbbRuleKey key, TMaybe<EExpBin> bin)
        : Key(key)
        , Bin(bin.GetOrElse(EExpBin::Count))
    {}

    auto operator<=>(const TBinnedCbbRuleKey& that) const = default;
};


} // namespace NAntiRobot


template <>
struct THash<NAntiRobot::TCbbRuleKey> {
    size_t operator()(NAntiRobot::TCbbRuleKey key) const {
        static_assert(
            sizeof(NAntiRobot::TCbbGroupId) + sizeof(NAntiRobot::TCbbRuleId) <= 8,
            "shift overflow in TCbbRuleKey hasher – please update"
        );

        return IntHash(
            (static_cast<ui64>(key.Group) << 32) |
            static_cast<ui64>(key.Rule)
        );
    }
};


template <>
struct THash<NAntiRobot::TBinnedCbbRuleKey> {
    size_t operator()(NAntiRobot::TBinnedCbbRuleKey key) const {
        return CombineHashes(
            THash<size_t>()(static_cast<size_t>(key.Bin)),
            THash<NAntiRobot::TCbbRuleKey>()(key.Key)
        );
    }
};
