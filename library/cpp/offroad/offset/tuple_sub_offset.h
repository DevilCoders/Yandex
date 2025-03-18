#pragma once

#include "data_offset.h"

namespace NOffroad {
    class TTupleSubOffset {
    public:
        TTupleSubOffset() = default;

        TTupleSubOffset(const TDataOffset& offset, ui32 subIndex)
            : Offset_(offset)
            , SubIndex_(subIndex)
        {
        }

        TDataOffset Offset() const {
            return Offset_;
        }

        ui32 SubIndex() const {
            return SubIndex_;
        }

        friend bool operator==(const TTupleSubOffset& l, const TTupleSubOffset& r) {
            return l.Offset_ == r.Offset_ && l.SubIndex_ == r.SubIndex_;
        }

        friend bool operator!=(const TTupleSubOffset& l, const TTupleSubOffset& r) {
            return l.Offset_ != r.Offset_ || l.SubIndex_ != r.SubIndex_;
        }

    private:
        TDataOffset Offset_;
        ui32 SubIndex_ = 0;
    };

    struct TTupleSubOffsetVectorizer {
        enum {
            TupleSize = 3
        };

        template <class Slice>
        Y_FORCE_INLINE static void Scatter(const TTupleSubOffset& data, Slice&& slice) {
            TUi64Vectorizer::Scatter(data.Offset().ToEncoded(), slice);
            slice[2] = data.SubIndex();
        }

        template <class Slice>
        Y_FORCE_INLINE static void Gather(Slice&& slice, TTupleSubOffset* data) {
            ui64 encoded;
            TUi64Vectorizer::Gather(slice, &encoded);
            *data = TTupleSubOffset(TDataOffset::FromEncoded(encoded), slice[2]);
        }
    };

    struct TTupleSubOffsetSubtractor {
        enum {
            TupleSize = 3,
            PrefixSize = 0
        };

        template <class Storage>
        Y_FORCE_INLINE static void Integrate(Storage&&) {
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Integrate(Value&& value, Delta&& delta, Next&& next) {
            next[0] = value[0] + delta[0];
            next[1] = delta[0] ? delta[1] : value[1] + delta[1];
            next[2] = value[2] + delta[2];
        }

        template <class Value, class Delta, class Next>
        Y_FORCE_INLINE static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
            delta[0] = next[0] - value[0];
            delta[1] = delta[0] ? next[1] : next[1] - value[1];
            delta[2] = next[2] - value[2];
        }
    };

    struct TTupleSubOffsetSerializer {
        enum {
            MaxSize = TUi64VarintSerializer::MaxSize * 2
        };

        Y_FORCE_INLINE static size_t Serialize(const TTupleSubOffset& data, ui8* output) {
            size_t size = TUi64VarintSerializer::Serialize(data.Offset().ToEncoded(), output);
            size += TUi64VarintSerializer::Serialize(data.SubIndex(), output + size);
            return size;
        }

        Y_FORCE_INLINE static size_t Deserialize(const ui8* input, TTupleSubOffset* data) {
            ui64 encoded;
            size_t size = TUi64VarintSerializer::Deserialize(input, &encoded);
            ui64 subIndex;
            size += TUi64VarintSerializer::Deserialize(input + size, &subIndex);

            *data = TTupleSubOffset(TDataOffset::FromEncoded(encoded), subIndex);

            return size;
        }
    };

}
