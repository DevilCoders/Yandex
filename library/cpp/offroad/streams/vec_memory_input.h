#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/array_ref.h>
#include <util/system/yassert.h>

#include <library/cpp/vec4/vec4.h>

namespace NOffroad {
    /**
     * An input stream that reads 4-vectors from provided memory region.
     *
     * @see TVecInput
     */
    class TVecMemoryInput: private TMoveOnly {
    public:
        /**
         * @param region                    Data region to read from.
         */
        TVecMemoryInput(const TArrayRef<const char>& region = TArrayRef<const char>()) {
            Reset(region);
        }

        /**
         * Assigns another buffer to this stream, resetting current position to zero.
         *
         * @param region                    Data region to read from.
         */
        inline void Reset(const TArrayRef<const char>& region = TArrayRef<const char>()) {
            Y_ASSERT(region.size() % sizeof(TVec4u) == 0);

            Begin_ = region.data();
            Pos_ = Begin_;
            End_ = Begin_ + region.size();
        }

        /**
         * @returns                         Next 4-vector from this stream.
         */
        inline TVec4u Read() {
            Y_ASSERT(Pos_ < End_);

            TVec4u result;
            result.Load(reinterpret_cast<const ui32*>(Pos_));

            Pos_ += sizeof(TVec4u);

            return result;
        }

        /**
         * Seeks into the provided position in this stream.
         *
         * @param position                  Position (bytes) to seek into.
         * @returns                         Whether seek operation was successful.
         *                                  A return value of false signals end of stream.
         */
        inline bool Seek(ui64 position) {
            const char* pos = Begin_ + position;
            if (pos >= End_)
                return false;

            Pos_ = pos;
            return true;
        }

        /**
         * @returns                         Current input position.
         */
        inline ui64 Position() const {
            return static_cast<ui64>(Pos_ - Begin_) * 2;
        }

        /**
         * @returns                         Whether there is nothing left to read.
         */
        inline bool Eof() const {
            return End_ == Pos_;
        }

    private:
        /** Buffer to read from. */
        const char* Begin_ = nullptr;

        /** Position in buffer. */
        const char* Pos_ = nullptr;

        /** End of buffer. */
        const char* End_ = nullptr;
    };

}
