#pragma once

#include "constraint_accessors.h"
#include "request_accessors.h"

namespace NReqBundle {
namespace NDetail {
    class TBlocksRemapper
        : public TVector<size_t>
    {
    public:
        TBlocksRemapper(size_t numBlocks = 0);
        void Reset(size_t numBlocks);

        bool CanRemap(TConstRequestAcc request) const;
        bool CanRemap(TConstConstraintAcc constraint) const;
        void Remap(TRequestAcc request) const;
        void Remap(TConstraintAcc constraint) const;

        void operator()(TRequestAcc request) const {
            Remap(request);
        }
        void operator()(TConstraintAcc constraint) const {
            Remap(constraint);
        }
    };
} // namespace NDetail
} // namespace NReqBundle
