#pragma once

#include "public.h"

#include <library/cpp/lwtrace/shuttle.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

enum class EProcessingStage
{
    Postponed,
    Backoff,
    Last
};

////////////////////////////////////////////////////////////////////////////////

struct TCallContext
    : TAtomicRefCount<TCallContext>
{
    ui64 RequestId;
    NLWTrace::TOrbit LWOrbit;
    TAtomic RequestStarted = 0;
    TAtomic Stage2Time[static_cast<int>(EProcessingStage::Last)] = {};
    TAtomic PostponeTsCycles = 0;
    TAtomic PostponeTs = 0;

    TCallContext(ui64 requestId = 0);

    TDuration CalcExecutionTime(ui64 startCycles, ui64 nowCycles) const;

    TDuration Time(EProcessingStage stage) const;

    void AddTime(EProcessingStage stage, TDuration d);
};

////////////////////////////////////////////////////////////////////////////////


}   // namespace NCloud::NBlockStore
