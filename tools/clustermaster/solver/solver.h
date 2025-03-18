#pragma once

#include "alloc.h"
#include "thingy.h"

#include <tools/clustermaster/communism/core/core.h>
#include <tools/clustermaster/communism/util/profile.h>

#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/network/pollerimpl.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/mutex.h>

#define PROFILE_STREAM Log(false)

enum {
    PROFILE_BATCH_PROCESS,
    PROFILE_BATCH_UPDATE,
    PROFILE_CLAIMS_COMBINE,
    PROFILE_CLAIMS_OUTPUT,
    PROFILE_EVENTS_PROCESS,
    PROFILE_EVENTS_WAIT,
    PROFILE_MAIN_SOLVING,
    PROFILE_MAIN_SOLVING_1,
    PROFILE_MAIN_SOLVING_2,
    PROFILE_MAIN_SOLVING_3,
    PROFILE_MAIN_SOLVING_4,
    PROFILE_MAIN_SOLVING_5,
    PROFILE_REQUEST_PARSE,
};

namespace NGlobal {

struct TPollerLockingPolicy {
    typedef TMutex TMyMutex;
};

typedef TPollerImpl<TPollerLockingPolicy> TPoller;

typedef TIntMapper<TString, unsigned> TKeyMapper;

struct TKnownLimits: THashMap<TString, double> {};

extern TAtomic NeedSolve;
extern TKeyMapper KeyMapper;
extern TKnownLimits KnownLimits;
extern TAtomic LastRequestIndexNumber;

}
