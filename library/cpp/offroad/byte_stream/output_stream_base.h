#pragma once

#include <library/cpp/offroad/codec/buffer_type.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/stream/output.h>

#include <cstring>

namespace NOffroad {
    template <class ByteEater, EBufferType bufferType>
    class TOutputStreamBase: public IOutputStream {
    public:
        using TByteEater = ByteEater;
        using TChunk = std::array<unsigned char, TByteEater::BlockSize>;
        using TPosition = TDataOffset;

        static constexpr EBufferType BufferType = bufferType;

        enum {
            BlockSize = TByteEater::BlockSize,
            Stages = TByteEater::Stages
        };

        template <class... Args>
        TOutputStreamBase(Args&&... args)
            : Eater_(std::forward<Args>(args)...)
        {
        }

        template <class... Args>
        void Reset(Args&&... args) {
            Eater_.Reset(std::forward<Args>(args)...);
            BytesInChunk_ = 0;
            Finished_ = false;
        }

        inline void Write(const void* buf, size_t len) {
            unsigned const char* buffer = AdaptBuffer(buf, len);

            size_t bytesWritten = AddToChunk<true>(buffer, len);
            FlushChunk();

            while (bytesWritten + BlockSize <= len) {
                Eater_.Write(0, TArrayRef<unsigned const char>(buffer + bytesWritten, BlockSize));
                bytesWritten += BlockSize;
            }

            AddToChunk<false>(buffer + bytesWritten, len - bytesWritten);
            FlushChunk();
        }

        bool IsFinished() const {
            return Finished_;
        }

        TDataOffset Position() {
            return TDataOffset(Eater_.Position(), BytesInChunk_);
        }

    protected:
        virtual void DoWrite(const void* buf, size_t len) override {
            Write(buf, len);
        }

        virtual void DoFlush() override {
            if (!BytesInChunk_)
                return;

            Fill(Chunk_.begin() + BytesInChunk_, Chunk_.end(), 0);
            Eater_.Write(0, Chunk_);
            BytesInChunk_ = 0;
        }

        virtual void DoFinish() override {
            Y_ENSURE(!Finished_);
            Flush();
            Finished_ = true;
            Eater_.Finish();
        }

    private:
        unsigned const char* AdaptBuffer(const void* buf, size_t len) const {
            unsigned const char* buffer = reinterpret_cast<unsigned const char*>(buf);
            if (BufferType == AutoEofBuffer) {
                Y_ENSURE(Find(buffer, buffer + len, 0) == buffer + len, "No zeroes allowed in AutoEofInputStream");
            }

            return buffer;
        }

        template <bool head>
        Y_FORCE_INLINE size_t AddToChunk(unsigned const char* buffer, size_t len) {
            if (BytesInChunk_ == 0 && head) {
                return 0;
            }

            const size_t bytesToWrite = Min(len, BlockSize - BytesInChunk_);

            memcpy(Chunk_.data() + BytesInChunk_, buffer, bytesToWrite);
            BytesInChunk_ += bytesToWrite;

            return bytesToWrite;
        }

        Y_FORCE_INLINE void FlushChunk() {
            if (BytesInChunk_ == BlockSize) {
                Eater_.Write(0, Chunk_);
                BytesInChunk_ = 0;
            }
        }

    protected:
        TByteEater Eater_;
        TChunk Chunk_;
        size_t BytesInChunk_ = 0;
        bool Finished_ = false;
    };

} //namespace NOffroad
