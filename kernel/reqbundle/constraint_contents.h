#pragma once

#include <kernel/lingboost/constants.h>

namespace NReqBundle {
namespace NDetail {
    struct TConstraintData {
        NLingBoost::EConstraintType Type = NLingBoost::TConstraint::Must;
        TVector<size_t> BlockIndices;
    };
} // namespace NDetail
} // namespace NReqBundle
