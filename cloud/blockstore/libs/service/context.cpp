#include "context.h"

#include <util/datetime/cputimer.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

TCallContext::TCallContext(ui64 requestId)
    : RequestId(requestId)
{
}

TDuration TCallContext::CalcExecutionTime(ui64 startCycles, ui64 nowCycles) const
{
    auto postponeDuration = TDuration::MicroSeconds(
        AtomicGet(Stage2Time[static_cast<int>(EProcessingStage::Postponed)]));
    ui64 postponeStart = AtomicGet(PostponeTsCycles);
    if (postponeStart && startCycles < postponeStart) {
        if (postponeDuration) {
            return CyclesToDurationSafe(nowCycles - startCycles) - postponeDuration;
        }
        return CyclesToDurationSafe(postponeStart - startCycles);
    }
    return CyclesToDurationSafe(nowCycles - startCycles);
}

TDuration TCallContext::Time(EProcessingStage stage) const
{
    return TDuration::MicroSeconds(AtomicGet(Stage2Time[static_cast<int>(stage)]));
}

void TCallContext::AddTime(EProcessingStage stage, TDuration d)
{
    AtomicAdd(Stage2Time[static_cast<int>(stage)], d.MicroSeconds());
}

}   // namespace NCloud::NBlockStore
