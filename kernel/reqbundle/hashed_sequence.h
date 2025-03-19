#pragma once

#include "reqbundle.h"
#include "serializer.h"

namespace NReqBundle {
namespace NDetail {
    class THashedSequenceAdapter
        : public TSequenceAcc
    {
    private:
        using THashValue = ui64;
        using TBlockHash = THashMap<ui64, size_t>;

    private:
        TBlockHash HashToIndex;
        TReqBundleSerializer Ser{TCompressorFactory::NO_COMPRESSION};

    public:
        THashedSequenceAdapter(TSequenceAcc seq)
            : TSequenceAcc(seq)
        {
            Rehash();
        }

        TSequenceAcc UnhashedSequence() const {
            return *this;
        }

        size_t AddElem(const TBlockPtr& block);
        size_t AddElem(const TBinaryBlockPtr& binary);
        size_t AddElem(TConstSequenceElemAcc elem);

        bool FindElem(TConstBlockAcc block, size_t& index) const;
        bool FindElem(TConstBinaryBlockAcc binary, size_t& index) const;
        bool FindElem(TConstSequenceElemAcc elem, size_t& index) const;

    private:
        size_t AddElemUnsafe(const TBlockPtr& block, THashValue hash);
        size_t AddElemUnsafe(const TBinaryBlockPtr& binary);
        bool FindElemByHash(THashValue hash, size_t& index) const;
        void Rehash();
    };
} // NDetail
} // NReqBundle
