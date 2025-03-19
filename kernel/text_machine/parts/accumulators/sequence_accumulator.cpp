#include "sequence_accumulator.h"

#include <kernel/text_machine/module/module_def.inc>

#include <util/generic/utility.h>
#include <util/generic/xrange.h>
#include <util/system/yassert.h>

namespace NTextMachine {
namespace NCore {
    void TSequenceAccumulator::Init(TMemoryPool& pool, ui16 numWords) {
        NumWords = numWords;
        QueryAnnPos.Init(pool, 2 * NumWords, EStorageMode::Full);
        Clear();
    }

    void TSequenceAccumulator::Clear() {
        QueryAnnPos.FillZeroes();
        MaxSequenceLength = 0;
    }

    void TSequenceAccumulator::Update(ui16 queryWordPos, ui16 annWordPos) {
        Y_ASSERT(queryWordPos < NumWords);
        ui16 curIndex = GetRecordIndex(queryWordPos, annWordPos);

        TQueryAnnPos& queryAnnPos = QueryAnnPos[curIndex];
        queryAnnPos.AnnWordPos = annWordPos;
        queryAnnPos.MaxSequenceLength = 1;

        if (queryWordPos > 0) {
            ui16 prevIndex = GetPrevRecordIndex(curIndex);
            if (QueryAnnPos[prevIndex].AnnWordPos + 1 == annWordPos) {
                queryAnnPos.MaxSequenceLength = QueryAnnPos[prevIndex].MaxSequenceLength + 1;
            }
        }

        MaxSequenceLength = Max(MaxSequenceLength, queryAnnPos.MaxSequenceLength);
    }

    size_t TSequenceAccumulator::GetMaxSequenceLength() {
        return MaxSequenceLength;
    }

    void TSequenceAccumulator::SaveToJson(NJson::TJsonValue& value) const {
        SAVE_JSON_VAR(value, MaxSequenceLength);
        SAVE_JSON_VAR(value, NumWords);
        SAVE_JSON_VAR(value, QueryAnnPos);
    }

    void TSequenceAccumulator::TQueryAnnPos::SaveToJson(NJson::TJsonValue& value) const {
        SAVE_JSON_VAR(value, AnnWordPos);
        SAVE_JSON_VAR(value, MaxSequenceLength);
    }
}; // NCore
}; // NTextMachine
