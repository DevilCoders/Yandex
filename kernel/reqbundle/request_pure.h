#pragma once

#include "sequence_accessors.h"
#include "request_accessors.h"

namespace NReqBundle {
    void FillRevFreqs(TRequestAcc request, TConstSequenceAcc seq, bool overwriteAll = false);
    void FillRevFreqs(TReqBundleAcc bundle, bool overwriteAll = false);
} // NReqBundle
