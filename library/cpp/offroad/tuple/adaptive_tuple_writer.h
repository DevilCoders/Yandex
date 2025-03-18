#pragma once

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/codec/bit_encoder_16.h>
#include <library/cpp/offroad/sub/sub_writer.h>

#include <util/stream/length.h>

#include "tuple_writer.h"

namespace NOffroad {
    template <class Data, class Vectorizer, class Subtractor, class PrefixVectorizer = TNullVectorizer>
    class TAdaptiveTupleWriter {
        using TVecBase = std::conditional_t<
            PrefixVectorizer::TupleSize == 0,
            TTupleWriter<Data, Vectorizer, Subtractor>,
            TSubWriter<PrefixVectorizer, TTupleWriter<Data, Vectorizer, Subtractor>>>;

        using TBitBase = TTupleWriter<Data, Vectorizer, Subtractor, TBitEncoder16, 4>;

        static_assert(std::is_same<typename TVecBase::TTable, typename TBitBase::TTable>::value, "Bit & vector table types must be the same.");
        static_assert(static_cast<ui32>(TVecBase::Stages) == static_cast<ui32>(TBitBase::Stages), "Bit & vector stages must be the same.");

    public:
        using THit = Data;
        using TTable = typename TVecBase::TTable;
        using TModel = typename TTable::TModel;

        enum {
            Stages = TVecBase::Stages
        };

        TAdaptiveTupleWriter()
            : DataOutput_(nullptr)
        {
            Buffer_.reserve(BitUsageLimit);
        }

        TAdaptiveTupleWriter(
            const TTable* table,
            IOutputStream* dataOutput,
            IOutputStream* subOutput = nullptr)
            : DataOutput_(nullptr)
        {
            Buffer_.reserve(BitUsageLimit);
            Reset(table, dataOutput, subOutput);
        }

        void Reset(
            const TTable* table,
            IOutputStream* dataOutput,
            IOutputStream* subOutput = nullptr)
        {
            Table_ = table;

            DataOutput_.~TCountingOutput();
            new (&DataOutput_) TCountingOutput(dataOutput);

            SubOutput_ = subOutput;

            Mode_ = PREPARED;
        }

        ~TAdaptiveTupleWriter() {
            Finish();
        }

        void WriteHit(const THit& data) {
            Y_ASSERT(Mode_ != FINISHED);

            if (Y_UNLIKELY(Mode_ == PREPARED)) {
                Mode_ = BIT_MODE;
            }

            if (Y_UNLIKELY(Buffer_.size() == BitUsageLimit)) {
                Mode_ = VEC_MODE;
                FlushBuffer();
            }

            if (Mode_ == VEC_MODE) {
                VecBase_.WriteHit(data);
            } else {
                Buffer_.emplace_back(data);
            }
        }

        bool IsFinished() const {
            return Mode_ == FINISHED;
        }

        void Finish() {
            if (IsFinished())
                return;

            if (Mode_ == BIT_MODE) {
                FlushBuffer();
                BitBase_.Finish();
                /* we should be sure that written size is not divisible by 16 */
                if (DataOutput_.Counter() % 16 == 0) {
                    DataOutput_.Write('\0');
                }
            } else if (Mode_ == VEC_MODE) {
                VecBase_.Finish();
            }

            Mode_ = FINISHED;
        }

    private:
        enum {
            BitUsageLimit = TBitBase::BlockSize
        };

        enum EState {
            PREPARED, // Reset was called
            BIT_MODE,
            VEC_MODE,
            FINISHED // Finish was called
        };

    private:
        void PrepareDataWrite() {
            Y_ASSERT(Mode_ == BIT_MODE || Mode_ == VEC_MODE);

            if (Mode_ == BIT_MODE) {
                BitBase_.Reset(Table_, &DataOutput_);
            } else if (Mode_ == VEC_MODE) {
                if constexpr (PrefixVectorizer::TupleSize != 0) {
                    VecBase_.Reset(SubOutput_, Table_, &DataOutput_);
                } else {
                    VecBase_.Reset(Table_, &DataOutput_);
                }
            }
        }

        void FlushBuffer() {
            Y_ASSERT(Buffer_);

            PrepareDataWrite();
            for (const THit& buffered : Buffer_) {
                (Mode_ == BIT_MODE) ? BitBase_.WriteHit(buffered) : VecBase_.WriteHit(buffered);
            }

            Buffer_.clear();
        }

    private:
        const TTable* Table_ = nullptr;

        TCountingOutput DataOutput_;
        IOutputStream* SubOutput_ = nullptr;

        EState Mode_ = FINISHED;
        TVector<THit> Buffer_;

        TVecBase VecBase_;
        TBitBase BitBase_;
    };

}
