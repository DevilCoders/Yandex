#pragma once

#include <type_traits>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/sub/sub_reader.h>
#include <library/cpp/offroad/codec/bit_decoder_16.h>

#include "tuple_reader.h"

namespace NOffroad {
    template <class Data, class Vectorizer, class Subtractor, class PrefixVectorizer = TNullVectorizer>
    class TAdaptiveTupleReader {
    private:
        using TVecBase = std::conditional_t<
            PrefixVectorizer::TupleSize == 0,
            TTupleReader<Data, Vectorizer, Subtractor>,
            TSubReader<PrefixVectorizer, TTupleReader<Data, Vectorizer, Subtractor>>>;

        using TBitBase = std::conditional_t<
            PrefixVectorizer::TupleSize == 0,
            TTupleReader<Data, Vectorizer, Subtractor, NOffroad::TBitDecoder16, 4>,
            TSubReader<PrefixVectorizer, TTupleReader<Data, Vectorizer, Subtractor, NOffroad::TBitDecoder16, 4>>>;

        static_assert(std::is_same<typename TVecBase::TTable, typename TBitBase::TTable>::value, "Bit & vector table types must be the same.");

    public:
        using THit = Data;
        using TTable = typename TVecBase::TTable;
        using TModel = typename TTable::TModel;

        TAdaptiveTupleReader() {
        }

        TAdaptiveTupleReader(
            const TTable* table,
            const TArrayRef<const char>& dataRegion,
            const TArrayRef<const char>& subRegion = TArrayRef<const char>())
        {
            Reset(table, dataRegion, subRegion);
        }

        void Reset() {
            Mode_ = BIT_MODE;
            BitBase_.Reset();
        }

        void Reset(
            const TTable* table,
            const TArrayRef<const char>& dataRegion,
            const TArrayRef<const char>& subRegion = TArrayRef<const char>())
        {
            Table_ = table;

            if (dataRegion.size() > 0 && dataRegion.size() % 16 == 0) {
                Mode_ = VEC_MODE;
                if constexpr (PrefixVectorizer::TupleSize != 0) {
                    VecBase_.Reset(subRegion, Table_, dataRegion);
                } else {
                    Y_UNUSED(subRegion);
                    VecBase_.Reset(Table_, dataRegion);
                }
            } else {
                Mode_ = BIT_MODE;
                if constexpr (PrefixVectorizer::TupleSize != 0) {
                    BitBase_.Reset(subRegion, Table_, dataRegion);
                } else {
                    Y_UNUSED(subRegion);
                    BitBase_.Reset(Table_, dataRegion);
                }
            }
        }

        bool ReadHit(THit* data) {
            return (Mode_ == BIT_MODE ? BitBase_.ReadHit(data) : VecBase_.ReadHit(data));
        }

        template <class Consumer>
        bool ReadHits(const Consumer& consumer) {
            return (Mode_ == BIT_MODE ? BitBase_.ReadHits(consumer) : VecBase_.ReadHits(consumer));
        }

        bool LowerBound(const THit& prefix, THit* first) {
            return (Mode_ == BIT_MODE && BitBase_.LowerBound(prefix, first)) || (Mode_ == VEC_MODE && VecBase_.LowerBound(prefix, first));
        }

    private:
        const TTable* Table_ = nullptr;

        enum EState {
            BIT_MODE,
            VEC_MODE
        };

        EState Mode_ = BIT_MODE;
        TBitBase BitBase_;
        TVecBase VecBase_;
    };

}
