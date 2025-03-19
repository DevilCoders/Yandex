#pragma once

#include "block.h"

class TPureContainer;

namespace NReqBundle {
    i64 CalcCompoundRevFreq(TConstBlockAcc block, NLingBoost::EWordFreqType type = NLingBoost::TRevFreq::Default);
    void LoadRevFreqsFromPure(TBlockAcc block, const TPureContainer& container);
    void LoadRevFreqsFromPure(TSequenceAcc sequence, const TPureContainer& container);
} // NReqBundle

