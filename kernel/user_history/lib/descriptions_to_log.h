#pragma once

#include <kernel/user_history/proto/user_history.pb.h>

namespace NPersonalization {
    NProto::TUserRecordsDescription ConstructLongClicksDescription();

    NProto::TUserRecordsDescription ConstructUserBodyRequestsDescription();
    NProto::TUserRecordsDescription ConstructUserBodyClicksDescription();

    NProto::TUserRecordsDescription ConstructUserBodyFreshRequestsDescription();
    NProto::TUserRecordsDescription ConstructUserBodyFreshClicksDescription();
}
