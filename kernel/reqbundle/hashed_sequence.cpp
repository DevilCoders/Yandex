#include "hashed_sequence.h"

namespace NReqBundle {
namespace NDetail {
    void THashedSequenceAdapter::Rehash()
    {
        for (size_t i : xrange(GetNumElems())) {
            auto elem = GetElem(i);
            if (elem.HasBinary()) {
                HashToIndex[elem.GetBinary().GetHash()] = i;
            } else {
                HashToIndex[Ser.GetBinaryHash(elem.GetBlock())] = i;
            }
        }
    }

    size_t THashedSequenceAdapter::AddElemUnsafe(const TBlockPtr& block, THashValue hash)
    {
        const size_t index = GetNumElems();
        NDetail::BackdoorAccess(*this).Elems.push_back(NDetail::SequenceElem(block));
        HashToIndex[hash] = index; // can overwrite in case of collision
        return index;
    }

    size_t THashedSequenceAdapter::AddElemUnsafe(const TBinaryBlockPtr& binary)
    {
        const size_t index = GetNumElems();
        NDetail::BackdoorAccess(*this).Elems.push_back(NDetail::SequenceElem(binary));
        HashToIndex[binary->GetHash()] = index; // can overwrite in case of collision
        return index;
    }

    bool THashedSequenceAdapter::FindElemByHash(THashValue hash, size_t& index) const
    {
        auto it = HashToIndex.find(hash);
        if (it != HashToIndex.end()) {
            index = it->second;
            return true;
        }
        return false;
    }

    bool THashedSequenceAdapter::FindElem(TConstBlockAcc block, size_t& index) const
    {
        return FindElemByHash(Ser.GetBinaryHash(block), index);
    }

    bool THashedSequenceAdapter::FindElem(TConstBinaryBlockAcc binary, size_t& index) const
    {
        return FindElemByHash(binary.GetHash(), index);
    }

    bool THashedSequenceAdapter::FindElem(TConstSequenceElemAcc elem, size_t& index) const
    {
        if (elem.HasBinary()) {
            return FindElem(elem.GetBinary(), index);
        } else {
            Y_ASSERT(elem.HasBlock());
            return FindElem(elem.GetBlock(), index);
        }
    }

    size_t THashedSequenceAdapter::AddElem(const TBlockPtr& block)
    {
        THashValue hash = Ser.GetBinaryHash(*block);
        size_t index = 0;
        if (FindElemByHash(hash, index)) {
            return index;
        }
        return AddElemUnsafe(block, hash);
    }

    size_t THashedSequenceAdapter::AddElem(const TBinaryBlockPtr& binary)
    {
        size_t index = 0;
        if (FindElem(*binary, index)) {
            return index;
        }
        return AddElemUnsafe(binary);
    }

    size_t THashedSequenceAdapter::AddElem(TConstSequenceElemAcc elem) {
        if (elem.HasBinary()) {
            return AddElem(NDetail::BackdoorAccess(elem).Binary);
        } else {
            Y_ASSERT(elem.HasBlock());
            return AddElem(NDetail::BackdoorAccess(elem).Block);
        }
    }
} // NDetail
} // NReqBundle
