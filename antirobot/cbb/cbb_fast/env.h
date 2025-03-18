#pragma once

#include "database.h"
#include "tvm.h"

#include <antirobot/cbb/cbb_fast/protos/config.pb.h>

#include <library/cpp/threading/rcu/rcu_accessor.h>

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/system/spinlock.h>

#include <array>

namespace NCbbFast {

constexpr size_t MAX_GROUPS = 4096;

struct TSharedGroup {
    NThreading::TRcuAccessor<TGroup> Group;
    TAdaptiveLock Lock;
};

class TEnv : TNonCopyable {
public:
    explicit TEnv(const TConfig& config);

public:
    TTvm Tvm;
    std::array<TSharedGroup, MAX_GROUPS> Groups;

    NThreading::TRcuAccessor<TUpdatedTimes> Updated;
    std::atomic<TInstant> UpdatedTs{};
    TAdaptiveLock UpdatedLock;

    TDatabase Database;
};

} // namespace NCbbFast
