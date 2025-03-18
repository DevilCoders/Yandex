#pragma once

#include <util/system/yassert.h>
#include <util/generic/array_ref.h>

#include <library/cpp/offroad/streams/bit_output.h>

#include "private/encoder_table.h"

#include "table_64.h"

namespace NOffroad {
    /**
     * Encoder that writes into a bit stream and operates on chunks of 16 integers.
     *
     * @see TEncoder16 for example code.
     */
    class TBitEncoder16 {
    public:
        enum {
            TupleSize = 1,
            BlockSize = 16,
            Stages = 1
        };

        using TTable = TEncoder64Table;
        using TOutput = TBitOutput;

        TBitEncoder16() {
        }

        TBitEncoder16(const TTable* table, TBitOutput* output) {
            Reset(table, output);
        }

        void Reset(const TTable* table, TBitOutput* output) {
            Table_ = table;
            Output_ = output;
        }

        template <class Range>
        void Write(size_t channel, const Range& chunk) {
            Write(channel, MakeArrayRef(chunk));
        }

        /**
         * Compresses and writes out provided data chunk of size 16 into the
         * underlying stream.
         */
        template <class Unsigned>
        void Write(size_t channel, const TArrayRef<const Unsigned>& chunk) {
            NPrivate::AssertSupportedType<Unsigned>();

            Y_ASSERT(chunk.size() == BlockSize);
            Y_ASSERT(channel == 0);
            Y_UNUSED(channel);

            ui32 levels[4];
            ui32 schemes[4];
            ui32 topLevel;
            ui32 topScheme;

            for (size_t i = 0; i < 4; ++i)
                Table_->Base1().CalculateParams(NPrivate::LoadVec(&chunk[i * 4]), &levels[i], &schemes[i]);

            Table_->Base0().CalculateParams(TVec4u(&schemes[0 * 4]), &topLevel, &topScheme);

            Output_->Write(topScheme, 8);
            Output_->Write(TVec4u(&schemes[0]) & VectorMask(topLevel), topLevel);

            size_t lev = 0;
            for (size_t i = 0; i < 4; ++i)
                lev += levels[i];

            if (lev <= 32) {
                size_t shift = 0;
                TVec4u res;
                for (size_t i = 0; i < 4; ++i) {
                    res = res | ((NPrivate::LoadVec(&chunk[i * 4]) & VectorMask(levels[i])) << shift);
                    shift += levels[i];
                }
                Output_->Write(res, shift);
            } else {
                for (size_t i = 0; i < 4; ++i) {
                    Output_->Write(NPrivate::LoadVec(&chunk[i * 4]) & VectorMask(levels[i]), levels[i]);
                }
            }
        }

        const TTable* Table() const {
            return Table_;
        }

    private:
        const TTable* Table_ = nullptr;
        TBitOutput* Output_ = nullptr;
    };

}
