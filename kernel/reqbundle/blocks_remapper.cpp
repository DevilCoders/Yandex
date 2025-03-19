#include "blocks_remapper.h"

namespace NReqBundle {
namespace NDetail {
    TBlocksRemapper::TBlocksRemapper(size_t numBlocks)
        : TVector<size_t>(numBlocks, Max<size_t>())
    {}
    void TBlocksRemapper::Reset(size_t numBlocks) {
        assign(numBlocks, Max<size_t>());
    }

    bool TBlocksRemapper::CanRemap(TConstRequestAcc request) const
    {
        for (auto match : request.GetMatches()) {
            const size_t index = match.GetBlockIndex();
            const size_t remappedIndex = (*this)[index];
            if (Max<size_t>() == remappedIndex) {
                return false;
            }
        }
        return true;
    }

    bool TBlocksRemapper::CanRemap(TConstConstraintAcc constraint) const
    {
        for (const size_t index : constraint.GetBlockIndices()) {
            const size_t remappedIndex = (*this)[index];
            if (Max<size_t>() == remappedIndex) {
                return false;
            }
        }
        return true;
    }

    void TBlocksRemapper::Remap(TRequestAcc request) const
    {
        Y_ASSERT(CanRemap(request));

        for (auto match : request.Matches()) {
            const size_t index = match.GetBlockIndex();
            const size_t remappedIndex = (*this)[index];
            NDetail::BackdoorAccess(match).BlockIndex = remappedIndex;
        }
    }

    void TBlocksRemapper::Remap(TConstraintAcc constraint) const
    {
        Y_ASSERT(CanRemap(constraint));

        const TVector<size_t>& indices = constraint.GetBlockIndices();
        for (size_t i = 0; i < indices.size(); i++) {
            const size_t index = indices[i];
            const size_t remappedIndex = (*this)[index];
            NDetail::BackdoorAccess(constraint).BlockIndices[i] = remappedIndex;
        }
    }
} // namespace NDetail
} // namespace NReqBundle
