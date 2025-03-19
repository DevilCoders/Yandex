#pragma once

#include "compression_type.h"
#include "struct_reader.h"
#include "struct_type.h"
#include "doc_data_holder.h"

#include <kernel/doom/wad/wad_index_type.h>


namespace NDoom {


template <class Data, class Serializer, EStructType structType, ECompressionType compressionType>
class TOffroadStructWadIterator {
public:
    using THit = Data;

    using TReader = TStructReader<Data, Serializer, structType, compressionType>;

    TOffroadStructWadIterator() = default;

    TOffroadStructWadIterator(const TSearchDocDataHolder* docDataHolder)
        : DocDataHolder_(docDataHolder)
    {}

    bool ReadHit(THit* hit) {
        return Reader_.Read(hit);
    }

private:
    template <EWadIndexType indexType, class OtherData, class OtherSerializer, EStructType otherStreamType, ECompressionType otherCompressionType>
    friend class TOffroadStructWadSearcher;

    TBlob Blob_;
    TReader Reader_;

    const TSearchDocDataHolder* DocDataHolder_ = nullptr;
};


} // namespace NDoom
