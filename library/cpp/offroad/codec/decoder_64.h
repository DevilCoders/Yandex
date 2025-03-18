#pragma once

#include <util/system/yassert.h>
#include <util/generic/array_ref.h>

#include <library/cpp/offroad/utility/masks.h>
#include <library/cpp/offroad/streams/vec_input.h>

#include "private/decoder_table.h"
#include "private/utility.h"

#include "table_64.h"

namespace NOffroad {
    /**
     * Decoder that reads from a vector stream and operates on chunks of 64 integers.
     *
     * @see TDecoder16 for usage example.
     */
    class TDecoder64 {
    public:
        enum {
            TupleSize = 1,
            BlockSize = 64,
        };

        using TTable = TDecoder64Table;
        using TInput = TVecInput;

        TDecoder64() = default;

        TDecoder64(const TTable* table, TVecInput* input) {
            Reset(table, input);
        }

        void Reset(const TTable* table, TVecInput* input) {
            Table_ = table;
            Input_ = input;
        }

        template <class Range>
        size_t Read(size_t channel, Range* chunk) {
            return Read(channel, MakeArrayRef(*chunk));
        }

        /**
         * Reads and decompresses the next data block of size 64 from the underlying
         * stream.
         *
         * @param[out] block                Chunk to decompress into.
         * @returns                         Total number of 4-bits read from the
         *                                  underlying stream. A return value of zero
         *                                  signals end of stream, other values are
         *                                  not really useful for the user.
         */
        template <class Unsigned>
        size_t Read(size_t channel, TArrayRef<Unsigned> block) {
            NPrivate::AssertSupportedType<Unsigned>();

            Y_ASSERT(block.size() == BlockSize);
            Y_ASSERT(channel == 0);
            Y_UNUSED(channel);

            size_t result = 0;

            TVec4u scheme0;
            result += Input_->Read(&scheme0, 8);
            if (result != 8) {
                return 0;
            }
            scheme0 = scheme0 & VectorMask(8);

            TVec4u schemes1[4];
            result += Table_->Base0().Read4(Input_, scheme0, &schemes1[0], &schemes1[1], &schemes1[2], &schemes1[3]);

            for (size_t i = 0; i < 4; ++i) {
                TVec4u r0, r1, r2, r3;
                result += Table_->Base1().Read4(Input_, schemes1[i], &r0, &r1, &r2, &r3);
                NPrivate::StoreVec4(r0, r1, r2, r3, &block[i * 16]);
            }

            return result;
        }

        const TTable* Table() const {
            return Table_;
        }

    private:
        const TTable* Table_ = nullptr;
        TVecInput* Input_ = nullptr;
    };

}
