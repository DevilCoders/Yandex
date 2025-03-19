#pragma once

#include "compression_type.h"
#include "struct_type.h"

#include <library/cpp/offroad/byte_stream/byte_input_stream.h>
#include <library/cpp/offroad/keyinv/null_model.h>

#include <util/generic/buffer.h>
#include <util/generic/fwd.h>
#include <util/generic/array_ref.h>
#include <util/generic/string.h>

#include <array>


namespace NDoom {


namespace NPrivate {


template<size_t chunkSize>
class TChunkedReader {
public :
    size_t ReadToBuffer(IInputStream* stream, size_t needReadSize, TBuffer* dest) {
        size_t readSize = 0;
        while (true) {
            size_t readRequest = Min(chunkSize, needReadSize);
            if (!readRequest)
                break;

            size_t currentChunkSize = stream->Read(Chunk_.data(), readRequest);
            dest->Append(Chunk_.data(), currentChunkSize);
            needReadSize -= currentChunkSize;
            readSize += currentChunkSize;

            if (currentChunkSize < readRequest)
                break;
        }

        return readSize;
    }

private :
    std::array<char, chunkSize> Chunk_;
};


}

template <EStructType structType, ECompressionType compressionType>
class TSerializedStructReader;

template <>
class TSerializedStructReader<FixedSizeStructType, OffroadCompressionType> {
public:
    using TStream = NOffroad::TByteInputStream;
    using TTable = TStream::TTable;

    void Reset(const TTable* table, const TArrayRef<const char>& region, ui32 indexStructSize, ui32 actualStructSize) {
        HasData_ = true;
        Stream_.Reset(table, region);
        Buffer_.Resize(actualStructSize);
        if (Y_UNLIKELY(indexStructSize < actualStructSize)) {
            memset(Buffer_.Data() + indexStructSize, 0, actualStructSize - indexStructSize);
        }
        StructSize_ = indexStructSize;
    }

    bool Read(TArrayRef<const char>* region) {
        if (!HasData_) {
            return false;
        }
        Stream_.Read(Buffer_.Data(), StructSize_);
        *region = TArrayRef<const char>(Buffer_.data(), Buffer_.size());
        HasData_ = false;
        return true;
    }

private:
    bool HasData_ = false;
    TStream Stream_;
    TBuffer Buffer_;
    ui32 StructSize_ = 0;
};

template <>
class TSerializedStructReader<FixedSizeStructType, RawCompressionType> {
public:
    using TTable = NOffroad::TNullTable;

    void Reset(const TTable* /*table*/, const TArrayRef<const char>& region, ui32 indexStructSize, ui32 actualStructSize) {
        HasData_ = true;
        if (Y_UNLIKELY(indexStructSize < actualStructSize || region.size() != actualStructSize)) {
            Buffer_.Resize(actualStructSize);
            const ui32 dataSize = Min(actualStructSize, indexStructSize, (ui32)region.size());
            memcpy(Buffer_.data(), region.data(), dataSize);
            memset(Buffer_.data() + indexStructSize, 0, actualStructSize - dataSize);
            Region_ = TArrayRef<const char>(Buffer_.data(), Buffer_.size());
        } else {
            Region_ = region;
        }
    }

    bool Read(TArrayRef<const char>* region) {
        if (!HasData_) {
            return false;
        }
        *region = Region_;
        HasData_ = false;
        return true;
    }

private:
    bool HasData_ = false;
    TArrayRef<const char> Region_;
    TBuffer Buffer_;
};

template <>
class TSerializedStructReader<AutoEofStructType, OffroadCompressionType> {
public:
    using TStream = NOffroad::TByteInputStreamEof;
    using TTable = TStream::TTable;

    void Reset(const TTable* table, const TArrayRef<const char>& region, ui32, ui32) {
        HasData_ = true;
        Stream_.Reset(table, region);
    }

    bool Read(TArrayRef<const char>* region) {
        if (!HasData_) {
            return false;
        }
        HasData_ = false;
        Buffer_.Clear();
        ChunkedReader_.ReadToBuffer(&Stream_, Max<size_t>(), &Buffer_);
        *region = TArrayRef<const char>(Buffer_.data(), Buffer_.size());
        return true;
    }

private:
    enum {
        ChunkSize = (1 << 15) // 32KB
    };

    bool HasData_ = false;
    TStream Stream_;
    NPrivate::TChunkedReader<ChunkSize> ChunkedReader_;
    TBuffer Buffer_;
};

template <>
class TSerializedStructReader<AutoEofStructType, RawCompressionType> {
public:
    using TTable = NOffroad::TNullTable;

    void Reset(const TTable* /*table*/, const TArrayRef<const char>& region, ui32, ui32) {
        HasData_ = true;
        Region_ = region;
    }

    bool Read(TArrayRef<const char>* region) {
        if (!HasData_) {
            return false;
        }
        HasData_ = false;
        *region = Region_;
        return true;
    }

private:
    bool HasData_ = false;
    TArrayRef<const char> Region_;
};

template <>
class TSerializedStructReader<VariableSizeStructType, OffroadCompressionType> {
public:
    using TStream = NOffroad::TByteInputStream;
    using TTable = TStream::TTable;

    void Reset(const TTable* table, const TArrayRef<const char>& region, ui32, ui32) {
        HasData_ = true;
        Table_ = table;
        Region_ = region;
    }

    bool Read(TArrayRef<const char>* region) {
        if (!HasData_) {
            return false;
        }
        HasData_ = false;

        Y_ENSURE(Region_.size() >= 1);
        ui8 lastBlockSize = static_cast<ui8>(Region_.data()[0]);
        Stream_.Reset(Table_, Region_.SubRegion(1, Region_.size() - 1));

        Buffer_.Clear();
        size_t readSize = ChunkedReader_.ReadToBuffer(&Stream_, Max<ui32>(), &Buffer_);
        Y_ENSURE(readSize % TStream::BlockSize == 0);

        ui32 structSize = readSize - (lastBlockSize == 0 ? 0 : TStream::BlockSize - lastBlockSize);
        *region = TArrayRef<const char>(Buffer_.data(), structSize);
        return true;
    }

private:
    enum {
        ChunkSize = (1 << 15) // 32KB
    };

    bool HasData_ = false;
    TStream Stream_;
    TArrayRef<const char> Region_;
    const TTable* Table_ = nullptr;
    NPrivate::TChunkedReader<ChunkSize> ChunkedReader_;
    TBuffer Buffer_;
};

template <>
class TSerializedStructReader<VariableSizeStructType, RawCompressionType> {
public:
    using TTable = NOffroad::TNullTable;

    void Reset(const TTable* /*table*/, const TArrayRef<const char>& region, ui32, ui32) {
        HasData_ = true;
        Region_ = region;
    }

    bool Read(TArrayRef<const char>* region) {
        if (!HasData_) {
            return false;
        }
        HasData_ = false;
        *region = Region_;
        return true;
    }

private:
    bool HasData_ = false;
    TArrayRef<const char> Region_;
};


} // namespace NDoom
