#pragma once

#include "compression_type.h"
#include "struct_type.h"

#include <kernel/doom/offroad_common/accumulating_output.h>
#include <kernel/doom/wad/wad.h>
#include <kernel/doom/wad/wad_writer.h>


namespace NDoom {


template <EWadIndexType indexType, class Data, class Serializer, EStructType structType, ECompressionType compressionType>
class TStructWriter;

template <EWadIndexType indexType, class Data, class Serializer, ECompressionType compressionType>
class TStructWriter<indexType, Data, Serializer, FixedSizeStructType, compressionType> {
    using TStream = std::conditional_t<
        compressionType == OffroadCompressionType,
        NOffroad::TByteOutputStream,
        NOffroad::TRawOutputStream
    >;

public:
    using TData = Data;
    using TTable = typename TStream::TTable;
    using TModel = typename TTable::TModel;

    TStructWriter() {
    }

    void Reset(const TTable* table) {
        Table_ = table;
        StructSize_ = Max<ui32>();
    }

    void Write(const TData& data, IOutputStream* out) {
        Stream_.Reset(Table_, out);

        ui32 size = Serializer::Serialize(data, &Stream_);
        Y_ENSURE(size < Max<ui32>());
        if (StructSize_ == Max<ui32>()) {
            StructSize_ = size;
        } else {
            Y_ENSURE(size == StructSize_);
        }

        Stream_.Finish();
    }

    void Finish(IWadWriter* wadWriter) {
        wadWriter->WriteGlobalLump(TWadLumpId(indexType, EWadLumpRole::StructSize), TArrayRef<const char>((const char*)&StructSize_, sizeof(StructSize_)));
    }

private:
    const TTable* Table_ = nullptr;

    TStream Stream_;
    ui32 StructSize_ = Max<ui32>();
};

template <EWadIndexType indexType, class Data, class Serializer, ECompressionType compressionType>
class TStructWriter<indexType, Data, Serializer, AutoEofStructType, compressionType>
{
    using TStream = std::conditional_t<
        compressionType == OffroadCompressionType,
        NOffroad::TByteOutputStreamEof,
        NOffroad::TRawOutputStream
    >;

public:
    using TData = Data;
    using TTable = typename TStream::TTable;
    using TModel = typename TTable::TModel;

    TStructWriter() {
    }

    void Reset(const TTable* table) {
        Table_ = table;
    }

    void Write(const TData& data, IOutputStream* out) {
        Stream_.Reset(Table_, out);
        Serializer::Serialize(data, &Stream_);
        Stream_.Finish();
    }

    void Finish(IWadWriter* /*wadWriter*/) {
    }

private:
    const TTable* Table_ = nullptr;
    IOutputStream* Output_ = nullptr;

    TStream Stream_;
};


template <EWadIndexType indexType, class Data, class Serializer>
class TStructWriter<indexType, Data, Serializer, VariableSizeStructType, RawCompressionType>
{
    using TStream = NOffroad::TRawOutputStream;

public:
    using TData = Data;
    using TTable = typename TStream::TTable;
    using TModel = typename TTable::TModel;

    TStructWriter() {
    }

    void Reset(const TTable* table) {
        Table_ = table;
    }

    void Write(const TData& data, IOutputStream* out) {
        Stream_.Reset(Table_, out);
        Serializer::Serialize(data, &Stream_);
        Stream_.Finish();
    }

    void Finish(IWadWriter* /*wadWriter*/) {
    }

private:
    const TTable* Table_ = nullptr;
    TStream Stream_;
};


template <EWadIndexType indexType, class Data, class Serializer>
class TStructWriter<indexType, Data, Serializer, VariableSizeStructType, OffroadCompressionType>
{
    using TStream = NOffroad::TByteOutputStream;

public:
    using TData = Data;
    using TTable = typename TStream::TTable;
    using TModel = typename TTable::TModel;

    TStructWriter() {
    }

    void Reset(const TTable* table) {
        Table_ = table;
    }

    void Write(const TData& data, IOutputStream* out) {
        ui32 size = Serializer::Serialize(data, &AccumulatingOutput_);
        Y_ENSURE(AccumulatingOutput_.Buffer().size() == size);

        static_assert(TStream::BlockSize <= Max<ui8>(), "Block size is too large.");
        ui8 lastBlockSize = static_cast<ui8>(size % TStream::BlockSize);
        out->Write(&lastBlockSize, 1);

        Stream_.Reset(Table_, out);
        AccumulatingOutput_.Flush(&Stream_);
        Stream_.Finish();
    }

    void Finish(IWadWriter* /*wadWriter*/) {
    }

private:
    const TTable* Table_ = nullptr;
    TAccumulatingOutput AccumulatingOutput_;

    TStream Stream_;
};


} // namespace NDoom
