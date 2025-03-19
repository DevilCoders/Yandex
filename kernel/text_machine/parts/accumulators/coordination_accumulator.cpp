#include "coordination_accumulator.h"

#include <util/system/yassert.h>
#include <util/generic/utility.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    void TCoordinationAccumulator::Init(TMemoryPool& pool, ui16 queryWordCount)
    {
        Y_ASSERT(queryWordCount > 0);
        Id = 0;
        LastIds.Init(pool, queryWordCount, EStorageMode::Full);
        LastIds.FillZeroes();
    }

    void TCoordinationAccumulator::SaveToJson(NJson::TJsonValue& value) const
    {
        SAVE_JSON_VAR(value, Id);
        SAVE_JSON_VAR(value, LastIds);
        SAVE_JSON_VAR(value, MatchCount);
    }

} // NCore
} // NTextMachine
