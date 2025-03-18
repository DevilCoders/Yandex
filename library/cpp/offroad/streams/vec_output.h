#pragma once

#include <util/stream/output.h>
#include <util/stream/null.h>
#include <util/system/yassert.h>

#include <library/cpp/vec4/vec4.h>
#include <library/cpp/offroad/utility/masks.h>

#include <array>

namespace NOffroad {
    /**
     * An output stream wrapper that writes 4-vectors of bits into the underlying stream.
     * The data will be laid out in the stream as follows:
     *
     * @code
     * stream.Write(TVec4u(0x0), 4);
     * stream.Write(TVec4u(0x0), 4);
     * stream.Write(TVec4u(0xF), 8);
     * stream.Write(TVec4u(0xA), 8);
     *
     * // Will produce:
     * // 0FA?0FA?0FA?0FA?
     * // Where '?' denotes the data that's yet to be written.
     * @endcode
     */
    class TVecOutput {
    public:
        TVecOutput(IOutputStream* stream = nullptr) {
            Reset(stream);
        }

        TVecOutput(const TVecOutput&) = delete;

        TVecOutput(TVecOutput&& other) {
            *this = std::move(other);
        }

        ~TVecOutput() {
            Finish();
        }

        TVecOutput& operator=(const TVecOutput&) = delete;

        TVecOutput& operator=(TVecOutput&& other) {
            memcpy(this, &other, sizeof(TVecOutput)); /* Ugly, but works. */
            other.Reset();
            return *this;
        }

        void Reset(IOutputStream* stream = nullptr) {
            Stream_ = stream ? stream : &Cnull;
            Ptr_ = 0;
            BitPos_ = 0;
            StreamPos_ = 0;
            Current_ = TVec4u();
            Finished_ = false;
        }

        /**
         * Writes the the provided number of least significant bits from each of
         * the elements of the provided 4-vector into this stream.
         *
         * @param data                      4-vector to write out.
         * @param bits                      Number of bits to write from each of the
         *                                  `data`'s elements.
         */
        void Write(const TVec4u& data, size_t bits) {
            if (bits == 0)
                return;

            Y_ASSERT(!Finished_);
            Y_ASSERT(bits <= 32);
            Y_ASSERT((data & VectorMask(bits)) == data);

            size_t end = BitPos_ + bits;
            Y_ASSERT(BitPos_ < 32);
            Current_ = Current_ | (data << BitPos_);
            if (end >= 32) {
                StreamPos_ += 32;
                if (Ptr_ == BufferSize)
                    Flush();

                Buffer_[Ptr_] = Current_;
                ++Ptr_;
                Current_ = (data >> (32 - BitPos_));
                end -= 32;
            }

            BitPos_ = end;
        }

        /**
         * Flushes all the data from this stream's buffer
         */
        void Flush() {
            if (Ptr_ != 0) {
                Stream_->Write(&Buffer_[0], sizeof(Buffer_[0]) * Ptr_);
                Ptr_ = 0;
            }
        }

        /**
         * Flushes all the data from this stream's buffer, pads it with zeros and
         * closes this stream. No more writing is possible once the stream is closed.
         */
        void Finish() {
            if (Finished_)
                return;

            if (BitPos_ != 0)
                Write(TVec4u(), 32 - BitPos_);

            Flush();
            Finished_ = true;

            Y_ASSERT(BitPos_ == 0);
        }

        /**
         * @returns                         Current output position, in format
         *                                  [offset:59][bits:5].
         */
        ui64 Position() const {
            return StreamPos_ + BitPos_;
        }

        /**
         * @returns                         Whether this stream is finished and no
         *                                  more writing is possible.
         */
        bool IsFinished() const {
            return Finished_;
        }

    private:
        enum {
            BufferSize = 16,
        };

        /** Underlying stream. */
        IOutputStream* Stream_ = nullptr;

        /** Internal buffer.*/
        std::array<TVec4u, BufferSize> Buffer_;

        /** Ptr for the internal buffer*/
        size_t Ptr_ = 0;

        /** Current 4 vec. */
        TVec4u Current_;

        /** Bit position in Current vec, [0...31]. */
        size_t BitPos_ = 0;

        /** Position in the underlying stream. */
        ui64 StreamPos_ = 0;

        bool Finished_ = false;
    };

}
