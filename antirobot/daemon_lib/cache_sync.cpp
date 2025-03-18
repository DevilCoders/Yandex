#include "cache_sync.h"

#include "blocker.h"
#include "uid.h"

#include <antirobot/idl/cache_sync.pb.h>

#include <utility>


namespace NAntiRobot {


void WriteBlockSyncResponse(
    const TBlocker& blocker,
    const TUid& uid,
    NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBlockAction>* actions
) {
    for (const auto& block : blocker.GetAllUidBlocks(uid)) {
        TStringOutput output(*actions->Add()->MutableData());
        Save(&output, block);
    }
}

void ApplyBlockSyncRequest(
    const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBlockAction>& actions,
    TBlocker* blocker
) {
    for (const auto& action : actions) {
        TBlockRecord block;

        TStringInput input(action.GetData());
        Load(&input, block);

        blocker->AddBlock(block);
    }
}

std::array<TRobotSet::TAllBans, HOST_NUMTYPES> ParseBanActions(
    const TUid& uid,
    const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBanAction>& actions
) {
    std::array<TRobotSet::TAllBans, HOST_NUMTYPES> serviceToBans;

    const auto now = TInstant::Now();

    for (const auto& action : actions) {
        const TUid actionUid(action.GetUid());
        const auto service = ParseHostType(action.GetHostType());
        TRobotSet::TAllBans bans(action, now);

        if (actionUid != uid) {
            continue;
        }

        serviceToBans[service] = std::move(bans);
    }

    return serviceToBans;
}


} // namespace NAntiRobot
