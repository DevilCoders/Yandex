#pragma once

#include <library/cpp/offroad/codec/bit_decoder_16.h>
#include <library/cpp/offroad/codec/buffer_type.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include <util/stream/output.h>
#include <util/stream/buffer.h>
#include <util/generic/array_ref.h>

namespace NOffroad {
    template <class Decoder, EBufferType bufferType>
    class TByteInputStreamBase: public IInputStream {
    public:
        using TDecoder = Decoder;
        using TTable = typename TDecoder::TTable;
        using TModel = typename TTable::TModel;
        using TChunk = std::array<unsigned char, TDecoder::BlockSize>;

        static constexpr EBufferType BufferType = bufferType;

        enum {
            BlockSize = TDecoder::BlockSize
        };

        TByteInputStreamBase() {
        }

        TByteInputStreamBase(const TTable* table, const TArrayRef<const char>& region)
            : Input_(region)
            , Decoder_(table, &Input_)
        {
        }

        void Reset(const TTable* table, const TArrayRef<const char>& region) {
            Input_.Reset(region);
            Decoder_.Reset(table, &Input_);
            StreamPosition_ = 0;
            Begin_ = 0;
            End_ = 0;
        }

        inline size_t Read(void* buf, size_t len) {
            unsigned char* buffer = reinterpret_cast<unsigned char*>(buf);

            size_t readBytes = CopyToBuffer<true>(buffer, len);

            while (readBytes + BlockSize <= len) {
                const size_t bytes = DecodeToBuffer(buffer + readBytes);
                readBytes += bytes;
                if (bytes != BlockSize) {
                    return readBytes;
                }
            }

            readBytes += CopyToBuffer<false>(buffer + readBytes, len - readBytes);

            Y_ASSERT(readBytes <= len);
            return readBytes;
        }

        TDataOffset Position() {
            if (Begin_ == End_) {
                return TDataOffset(Input_.Position(), 0);
            } else {
                return TDataOffset(StreamPosition_, Begin_);
            }
        }

        bool Seek(TDataOffset position) {
            const ui64 streamPosition = position.Offset();
            const size_t blockPosition = position.Index();

            if (streamPosition != StreamPosition_ || End_ == 0) {
                //then we need to decode new chunk.
                if (!Input_.Seek(streamPosition)) {
                    return false;
                }
                if (!RefreshChunk()) {
                    return false;
                }
            }

            if (blockPosition >= End_) {
                return false;
            } else {
                Begin_ = blockPosition;
                return true;
            }
        }

    protected:
        virtual size_t DoRead(void* buf, size_t len) override {
            return Read(buf, len);
        }

    private:
        Y_FORCE_INLINE size_t DecodeToBuffer(unsigned char* buffer) {
            auto chunk = TArrayRef<unsigned char>(buffer, BlockSize);

            if (!Decoder_.Read(0, chunk)) {
                return 0;
            }

            StreamPosition_ = Input_.Position();
            return ZeroesPosition(chunk);
        }

        Y_FORCE_INLINE bool RefreshChunk() {
            if (!Decoder_.Read(0, &Chunk_)) {
                return false;
            }

            Begin_ = 0;
            End_ = ZeroesPosition(Chunk_);
            StreamPosition_ = Input_.Position();
            return true;
        }

        Y_FORCE_INLINE size_t ZeroesPosition(const TArrayRef<unsigned char>& chunk) const {
            if (BufferType == AutoEofBuffer && chunk.back() == '\0') {
                size_t position = chunk.size() - 1;
                while (position > 0 && chunk[position - 1] == '\0') {
                    --position;
                }
                return position;
            }
            return BlockSize;
        }

        template <bool head>
        Y_FORCE_INLINE size_t CopyToBuffer(unsigned char* buffer, size_t len) {
            if (Begin_ == End_ && (head || !RefreshChunk())) {
                return 0;
            }

            Y_ASSERT(Begin_ < End_);
            Y_ASSERT(End_ <= BlockSize);

            const size_t hasBytes = Min(len, End_ - Begin_);

            memcpy(buffer, Chunk_.data() + Begin_, hasBytes);
            Begin_ += hasBytes;

            return hasBytes;
        }

    private:
        TBitInput Input_;
        TDecoder Decoder_;
        ui64 StreamPosition_ = 0;
        size_t Begin_ = 0;
        size_t End_ = 0;
        TChunk Chunk_;
    };

    using TByteInputStream = TByteInputStreamBase<TBitDecoder16, PlainOldBuffer>;
    using TByteInputStreamEof = TByteInputStreamBase<TBitDecoder16, AutoEofBuffer>;

} //namespace NOffroad
