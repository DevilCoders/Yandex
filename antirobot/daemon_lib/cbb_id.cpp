#include "cbb_id.h"

#include <antirobot/idl/cache_sync.pb.h>
#include <antirobot/lib/enum.h>

#include <util/generic/cast.h>
#include <util/stream/output.h>
#include <util/string/cast.h>


namespace NAntiRobot {


TCbbRuleKey::TCbbRuleKey(const NCacheSyncProto::TCbbRule& pbRule)
    : Group{pbRule.GetGroup()}
    , Rule{pbRule.GetEntry()}
{}

void TCbbRuleKey::SerializeTo(NCacheSyncProto::TCbbRule* pbRule) const {
    pbRule->SetGroup(EnumValue(Group));
    pbRule->SetEntry(EnumValue(Rule));
}


} // namespace NAntiRobot


template <>
void Out<NAntiRobot::TCbbGroupId>(IOutputStream& out, NAntiRobot::TCbbGroupId group) {
    out << NAntiRobot::EnumValue(group);
}


template <>
NAntiRobot::TCbbGroupId FromStringImpl<NAntiRobot::TCbbGroupId>(const char* data, size_t len) {
    return SafeIntegerCast<NAntiRobot::TCbbGroupId>(
        FromStringImpl<NAntiRobot::TEnumValueType<NAntiRobot::TCbbGroupId>>(data, len)
    );
}


template <>
void Out<NAntiRobot::TCbbRuleId>(IOutputStream& out, NAntiRobot::TCbbRuleId rule) {
    out << NAntiRobot::EnumValue(rule);
}


template <>
void Out<NAntiRobot::TCbbRuleKey>(IOutputStream& out, const NAntiRobot::TCbbRuleKey& key) {
    out << key.Group << '#' << key.Rule;
}


template <>
void Out<NAntiRobot::TBinnedCbbRuleKey>(IOutputStream& out, const NAntiRobot::TBinnedCbbRuleKey& binnedKey) {
    out << binnedKey.Key;

    if (binnedKey.Bin != NAntiRobot::EExpBin::Count) {
        out << "#bin=" << binnedKey.Bin;
    }
}
