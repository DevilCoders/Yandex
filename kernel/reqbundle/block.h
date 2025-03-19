#pragma once

#include "block_contents.h"
#include "block_accessors.h"

namespace NReqBundle {
    class TBlock
        : public TAtomicRefCount<TBlock>
        , public TBlockAcc
    {
    private:
        NDetail::TBlockData Data;

    public:
        TBlock()
            : TBlockAcc(Data)
        {}
        TBlock(const NDetail::TBlockData& data)
            : TBlockAcc(Data)
            , Data(data)
        {}
        TBlock(TConstBlockAcc other)
            : TBlock(NDetail::BackdoorAccess(other))
        {}
        TBlock(const TBlock& other)
            : TBlock(other.Data)
        {}

        TBlock(const TRichRequestNode& node)
            : TBlockAcc(Data)
        {
            SetRichNode(node);
        }
        TBlock(const TWordNode& node)
            : TBlockAcc(Data)
        {
            SetWordNode(node);
        }

        TBlock& operator=(const TBlock& other) {
            Data = other.Data;
            return *this;
        }
    };

    class TBinaryBlock
        : public TAtomicRefCount<TBinaryBlock>
        , public TBinaryBlockAcc
    {
    private:
        NDetail::TBinaryBlockData Data;

    public:
        TBinaryBlock()
            : TBinaryBlockAcc(Data)
        {}
        TBinaryBlock(const NDetail::TBinaryBlockData& data)
            : TBinaryBlockAcc(Data)
            , Data(data)
        {}
        TBinaryBlock(TConstBinaryBlockAcc other)
            : TBinaryBlock(NDetail::BackdoorAccess(other))
        {}
        TBinaryBlock(const TBinaryBlock& other)
            : TBinaryBlock(other.Data)
        {}

        TBinaryBlock& operator=(const TBinaryBlock& other) {
            Data = other.Data;
            return *this;
        }
    };
} // NReqBundle

template<>
struct THash<NReqBundle::TConstBinaryBlockAcc> {
    inline size_t operator()(NReqBundle::TConstBinaryBlockAcc binary) const {
        return binary.GetHash();
    }
};

template<>
struct TEqualTo<NReqBundle::TConstBinaryBlockAcc> {
    inline bool operator()(NReqBundle::TConstBinaryBlockAcc binaryX, NReqBundle::TConstBinaryBlockAcc binaryY) const {
        return binaryX.GetHash() == binaryY.GetHash() && binaryX.Size() == binaryY.Size() &&
            0 == memcmp(binaryX.GetData().AsCharPtr(), binaryY.GetData().AsCharPtr(), binaryX.Size());
    }
};

