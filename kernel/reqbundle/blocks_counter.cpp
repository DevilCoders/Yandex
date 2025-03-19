#include "blocks_counter.h"

namespace NReqBundle {
namespace NDetail {
    TBlocksCounter::TBlocksCounter(size_t numBlocks)
        : TVector<size_t>(numBlocks, 0)
    {}
    void TBlocksCounter::Reset(size_t numBlocks) {
        assign(numBlocks, 0);
    }

    void TBlocksCounter::Add(TConstRequestAcc request)
    {
        for (auto match : request.GetMatches()) {
            (*this)[match.GetBlockIndex()] += 1;
        }
    }

    void TBlocksCounter::Add(TConstConstraintAcc constraint)
    {
        for (const size_t blockIdx : constraint.GetBlockIndices()) {
            (*this)[blockIdx]++;
        }
    }
} // namespace NDetail
} // namespace NReqBundle
