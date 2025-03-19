#pragma once

#include "constraint.h"
#include "sequence.h"
#include "request.h"

#include <util/generic/deque.h>

namespace NReqBundle {
namespace NDetail {
    struct TReqBundleData {
        TSequencePtr Sequence;
        TDeque<TRequestPtr> Requests;
        TDeque<TConstraintPtr> Constraints;
    };

    void ReplaceSequence(TReqBundleData& data, const TSequencePtr& seqPtr);
} // NDetail
} // NReqBundle
