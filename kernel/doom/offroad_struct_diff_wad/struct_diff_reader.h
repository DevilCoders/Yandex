#pragma once

#include <library/cpp/offroad/codec/decoder_64.h>
#include <library/cpp/offroad/codec/interleaved_decoder.h>
#include <util/system/unaligned_mem.h>

#include "struct_diff_common.h"
#include "struct_diff_input_buffer.h"


namespace NDoom {


template <class Key, class KeyVectorizer, class KeySubtractor, class Data>
class TStructDiffReader {
    using TBuffer = TStructDiffInputBuffer<NOffroad::TDecoder64::BlockSize, Key, KeyVectorizer, KeySubtractor, Data>;
public:
    using TKey = Key;
    using TData = Data;
    using TModel = TStructDiffModel<KeyVectorizer::TupleSize, sizeof(TData), NOffroad::TDecoder64::TTable::TModel>;
    using TTable = TStructDiffTable<KeyVectorizer::TupleSize, sizeof(TData), NOffroad::TDecoder64::TTable>;

    TStructDiffReader() {

    }

    TStructDiffReader(ui32 dataSize, const TTable* table, const TBlob& blob)
        : Input_(blob)
        , Decoder_(table, &Input_)
        , Buffer_(dataSize)
    {

    }

    void Reset(ui32 dataSize, const TTable* table, const TBlob& blob) {
        Input_.Reset(blob);
        Decoder_.Reset(table, &Input_);

        Buffer_.Reset(dataSize);
    }

    bool Seek(ui64 position, const TKey& lastKey) {
        if (!Input_.Seek(position)) {
            return false;
        }
        return Buffer_.FillBlock(&Decoder_, lastKey);
    }

    bool LowerBoundLocal(const TKey& key, TKey* firstKey, const TData** firstData) {
        return Buffer_.LowerBoundLocal(key, firstKey, firstData);
    }

    bool Read(TKey* key, const TData** data) {
        if (Buffer_.IsDone()) {
            if (!Buffer_.FillBlock(&Decoder_, Buffer_.LastKey())) {
                return false;
            }
        }
        Buffer_.Read(key, data);
        return true;
    }

private:
    NOffroad::TVecInput Input_;
    NOffroad::TInterleavedDecoder<TTable::TupleSize, NOffroad::TDecoder64> Decoder_;
    TBuffer Buffer_;
};


} // namespace NDoom
