#pragma once

#include "constraint_accessors.h"
#include "request_accessors.h"

namespace NReqBundle {
namespace NDetail {
    class TBlocksCounter
        : public TVector<size_t>
    {
    public:
        TBlocksCounter(size_t numBlocks = 0);
        void Reset(size_t numBlocks);
        void Add(TConstRequestAcc request);
        void Add(TConstConstraintAcc constraint);

        void operator()(TConstRequestAcc request) {
            Add(request);
        }
        void operator()(TConstConstraintAcc constraint) {
            Add(constraint);
        }
    };
} // namespace NDetail
} // namespace NReqBundle
