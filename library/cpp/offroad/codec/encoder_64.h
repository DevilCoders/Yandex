#pragma once

#include <util/system/yassert.h>
#include <util/generic/array_ref.h>

#include <library/cpp/offroad/streams/vec_output.h>

#include "private/encoder_table.h"

#include "table_64.h"

namespace NOffroad {
    /**
     * Encoder that writes into a vector stream and operates on chunks of 64 integers.
     *
     * @see TEncoder16 for usage example.
     */
    class TEncoder64 {
    public:
        enum {
            TupleSize = 1,
            BlockSize = 64,
            Stages = 1
        };

        using TTable = TEncoder64Table;
        using TOutput = TVecOutput;

        TEncoder64() = default;

        TEncoder64(const TTable* table, TVecOutput* output) {
            Reset(table, output);
        }

        void Reset(const TTable* table, TVecOutput* output) {
            Table_ = table;
            Output_ = output;
        }

        template <class Range>
        void Write(size_t channel, const Range& chunk) {
            Write(channel, MakeArrayRef(chunk));
        }

        /**
         * Compresses and writes out provided data block of size 64 unto the
         * underlying stream.
         */
        template <class Unsigned>
        void Write(size_t channel, const TArrayRef<const Unsigned>& block) {
            NPrivate::AssertSupportedType<Unsigned>();

            Y_ASSERT(block.size() == BlockSize);
            Y_ASSERT(channel == 0);
            Y_UNUSED(channel);

            ui32 levels[16];
            ui32 schemes[16];
            ui32 topLevels[4];
            ui32 topSchemes[4];

            for (size_t i = 0; i < 16; ++i)
                Table_->Base1().CalculateParams(NPrivate::LoadVec(&block[i * 4]), &levels[i], &schemes[i]);
            for (size_t i = 0; i < 4; ++i)
                Table_->Base0().CalculateParams(TVec4u(&schemes[i * 4]), &topLevels[i], &topSchemes[i]);

            Output_->Write(TVec4u(&topSchemes[0]), 8);
            for (size_t i = 0; i < 4; ++i)
                Output_->Write(TVec4u(&schemes[i * 4]) & VectorMask(topLevels[i]), topLevels[i]);
            for (size_t i = 0; i < 16; ++i)
                Output_->Write(NPrivate::LoadVec(&block[i * 4]) & VectorMask(levels[i]), levels[i]);
        }

        const TTable* Table() const {
            return Table_;
        }

    private:
        const TTable* Table_ = nullptr;
        TVecOutput* Output_ = nullptr;
    };

}
