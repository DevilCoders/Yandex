#pragma once

#include "compression_type.h"
#include "serialized_struct_reader.h"
#include "struct_type.h"


namespace NDoom {


template <class Data, class Serializer, EStructType structType, ECompressionType compressionType>
class TStructReader {
    static constexpr bool IsFixedSizeStruct = (structType == FixedSizeStructType);

    using TSerializedReader = TSerializedStructReader<structType, compressionType>;
public:
    using TData = Data;
    using TTable = typename TSerializedReader::TTable;
    using TModel = typename TTable::TModel;

    TStructReader() {
    }

    void Reset(const TTable* table, ui32 structSize, const TArrayRef<const char>& region) {
        if (IsFixedSizeStruct && Serializer::DataSize != -1) {
            SerializedReader_.Reset(table, region, structSize, Serializer::DataSize);
        } else {
            SerializedReader_.Reset(table, region, structSize, structSize);
        }
    }

    bool Read(TData* data) {
        TArrayRef<const char> region;
        if (!SerializedReader_.Read(&region)) {
            return false;
        }
        Serializer::Deserialize(region, data);
        return true;
    }

private:
    TSerializedReader SerializedReader_;
};


} // namespace NDoom
