#pragma once

#include "robot_set.h"

#include <google/protobuf/repeated_field.h>


namespace NAntiRobot {


namespace NCacheSyncProto {
    class TBlockAction;
    class TBanAction;
}

class TBlocker;
struct TUid;


void WriteBlockSyncResponse(
    const TBlocker& blocker,
    const TUid& uid,
    NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBlockAction>* actions
);

void ApplyBlockSyncRequest(
    const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBlockAction>& actions,
    TBlocker* blocker
);

std::array<TRobotSet::TAllBans, HOST_NUMTYPES> ParseBanActions(
    const TUid& uid,
    const NProtoBuf::RepeatedPtrField<NCacheSyncProto::TBanAction>& actions
);


} // namespace NAntiRobot
