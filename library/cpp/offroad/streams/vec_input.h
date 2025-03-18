#pragma once

#include <util/generic/array_ref.h>
#include <util/memory/blob.h>
#include <util/system/yassert.h>

#include <library/cpp/vec4/vec4.h>

#include "vec_memory_input.h"

namespace NOffroad {
    /**
     * An input stream wrapper that reads 4-vectors of bits from the underlying
     * memory source.
     *
     * @see TVecOutput
     */
    class TVecInput {
    public:
        TVecInput() {
            Reset(TArrayRef<const char>());
        }

        TVecInput(const TArrayRef<const char>& region) {
            Reset(region);
        }

        TVecInput(const TBlob& blob) {
            Reset(blob);
        }

        /* Generated copy & move ctors are OK. */

        void Reset(const TArrayRef<const char>& region) {
            Bits_ = 32;

            Input_.Reset(region);
        }

        void Reset(const TBlob& blob) {
            Bits_ = 32;

            Input_.Reset(TArrayRef<const char>(static_cast<const char*>(blob.Data()), blob.Size()));
        }

        /**
         * Reads the requested number of bits from this stream, writing them into
         * least significant bits of the elements of the provided 4-vector.
         *
         * Note that other bits of the provided 4-vector will be filled with
         * unspecified values as a side effect and should be manually discarded
         * by the user.
         *
         * @param[out] data                 4-vector to read into.
         * @param bits                      Number of bits to read into each
         *                                  element of `data`.
         * @returns                         Total number of bits read into each
         *                                  element of `data`. A return value
         *                                  of zero signals end of stream.
         */
        inline size_t Read(TVec4u* data, size_t bits) {
            Y_ASSERT(bits <= 32);

            TVec4u r = (Current_ >> Bits_); // (Current_ >> 32) - it's ok, see https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm_srli_epi32&expand=4874,5086
            size_t newBits = Bits_ + bits;
            if (newBits >= 32) {
                if (Input_.Eof()) {
                    const size_t bitsRead = 32 - Bits_;
                    Bits_ = 32;
                    *data = r;
                    return bitsRead;
                }

                Current_ = Input_.Read();
                r = r | (Current_ << (32 - Bits_)); // (Current_ << 32) - it's ok, see https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=_mm_srli_epi32&expand=4874,5086
                newBits -= 32;
            }

            Bits_ = newBits;
            *data = r;
            return bits;
        }

        /**
         * Seeks into the provided position in this stream.
         *
         * @param position                  Position to seek into.
         * @returns                         Whether seek operation was successful.
         */
        bool Seek(ui64 position) {
            const static ui64 BitMask = static_cast<ui64>(31);
            const static ui64 ByteMask = ~BitMask;

            if (!Input_.Seek((position & ByteMask) >> 1)) {
                return false;
            }

            Current_ = Input_.Read();
            Bits_ = position & BitMask;
            return true;
        }

        /**
         * @returns                         Current input position, in format
         *                                  [offset:59][bits:5].
         */
        ui64 Position() const {
            return Input_.Position() + Bits_ - sizeof(TVec4u) * 2;
        }

    private:
        /** Current vector being read. */
        TVec4u Current_;

        /** Number of bits already read from current vector. */
        size_t Bits_ = 32;

        /** Stream that reads 4-vectors. */
        TVecMemoryInput Input_;
    };

}
