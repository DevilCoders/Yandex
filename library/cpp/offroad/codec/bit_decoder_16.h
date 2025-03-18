#pragma once

#include <util/system/yassert.h>
#include <util/generic/array_ref.h>

#include <library/cpp/offroad/streams/bit_input.h>

#include "private/decoder_table.h"
#include "private/utility.h"

#include "table_64.h"

namespace NOffroad {
    /**
     * Decoder that reads from a bit stream and operates on blocks of 16 integers.
     *
     * @see TEncoder16 for example code.
     */
    class TBitDecoder16 {
    public:
        enum {
            TupleSize = 1,
            BlockSize = 16,
        };

        using TTable = TDecoder64Table;
        using TInput = TBitInput;

        TBitDecoder16() = default;

        TBitDecoder16(const TTable* table, TBitInput* input) {
            Reset(table, input);
        }

        void Reset(const TTable* table, TBitInput* input) {
            Table_ = table;
            Input_ = input;
        }

        template <class Range>
        size_t Read(size_t channel, Range* chunk) {
            return Read(channel, MakeArrayRef(*chunk));
        }

        /**
         * Reads and decompresses the next data chunk of size 16 from the underlying
         * stream.
         *
         * @param channel                   Channel to read from, should always be 0.
         * @param[out] chunk                Chunk to decompress into.
         * @returns                         Total number of bits read from the
         *                                  underlying stream. A return value of zero
         *                                  signals end of stream, other values are
         *                                  not really useful for the user.
         */
        template <class Unsigned>
        size_t Read(size_t channel, TArrayRef<Unsigned> chunk) {
            NPrivate::AssertSupportedType<Unsigned>();

            Y_ASSERT(channel == 0);
            Y_ASSERT(chunk.size() == BlockSize);
            Y_UNUSED(channel);

            size_t result = 0;

            ui64 scheme0;
            result += Input_->Read(&scheme0, 8);
            if (result != 8)
                return 0;

            scheme0 = scheme0 & ScalarMask(8);

            TVec4u schemes1;
            result += Table_->Base0().Read1(Input_, scheme0, &schemes1);

            {
                TVec4u r0, r1, r2, r3;
                result += Table_->Base1().Read4(Input_, schemes1, &r0, &r1, &r2, &r3);
                NPrivate::StoreVec4(r0, r1, r2, r3, &chunk[0]);
            }

            return result;
        }

        const TTable* Table() const {
            return Table_;
        }

    private:
        const TTable* Table_ = nullptr;
        TBitInput* Input_ = nullptr;
    };

}
