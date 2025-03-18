#pragma once

#include <util/generic/array_ref.h>
#include <util/system/yassert.h>

#include <library/cpp/offroad/streams/vec_input.h>

#include "private/decoder_table.h"
#include "private/utility.h"

#include "table_16.h"

namespace NOffroad {
    /**
     * Decoder that reads from a vector stream and operates on chunks of 16 integers.
     *
     * Example usage:
     * @code
     * TModel16 model = YourModel();
     *
     * IInputStream* stream = YourStream();
     * TVecInput input(stream);
     *
     * TDecoder16 decompressor(model, &input);
     *
     * std::array<ui32, 16> chunk;
     * while(decompressor.Read(&chunk))
     *     YouDoSomethingWithIt(chunk);
     * @endcode
     */
    class TDecoder16 {
    public:
        enum {
            TupleSize = 1,
            BlockSize = 16,
        };

        using TTable = TDecoder16Table;
        using TInput = TVecInput;

        TDecoder16() = default;

        TDecoder16(const TTable* table, TVecInput* input) {
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
         * Reads and decompresses the next data block of size 16 from the underlying
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

            TVec4u scheme;
            result += Input_->Read(&scheme, 8);
            if (result != 8) {
                return 0;
            }
            scheme = scheme & VectorMask(8);

            TVec4u r0, r1, r2, r3;
            result += Table_->Base().Read4(Input_, scheme, &r0, &r1, &r2, &r3);

            NPrivate::StoreVec4(r0, r1, r2, r3, block.data());

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
