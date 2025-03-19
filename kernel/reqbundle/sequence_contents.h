#pragma once

#include "block.h"

#include <util/generic/deque.h>

namespace NReqBundle {
namespace NDetail {
    struct TSequenceElemData {
        TBlockPtr Block;
        TBinaryBlockPtr Binary;
    };

    inline TSequenceElemData SequenceElem(const TBlockPtr& block) {
        Y_ASSERT(block);
        TSequenceElemData res;
        res.Block = block;
        return res;
    }

    inline TSequenceElemData SequenceElem(const TBinaryBlockPtr& binary) {
        Y_ASSERT(binary);
        TSequenceElemData res;
        res.Binary = binary;
        return res;
    }

    struct TSequenceData {
        TDeque<TSequenceElemData> Elems;
    };

    void PrepareBlock(TSequenceElemData& data, TReqBundleDeserializer& deser);
    void PrepareBinary(TSequenceElemData& data, TReqBundleSerializer& ser);
} // NDetail
} // NReqBundle

