#pragma once

#include <util/generic/ylimits.h>
#include <util/system/yassert.h>

#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/custom/ui64_varint_serializer.h>

namespace NOffroad {
    class TDataOffset {
    public:
        TDataOffset()
            : Position_(0)
        {
        }

        TDataOffset(ui64 offset, size_t index)
            : Position_((offset << 6) + index)
        {
            Y_ASSERT(index < 64);
        }

        static TDataOffset FromEncoded(ui64 position) {
            TDataOffset result;
            result.Position_ = position;
            return result;
        }

        static TDataOffset Max() {
            return TDataOffset::FromEncoded(::Max());
        }

        ui64 ToEncoded() const {
            return Position_;
        }

        ui64 Offset() const {
            return Position_ >> 6;
        }

        size_t Index() const {
            return Position_ & 63;
        }

        friend TDataOffset operator+(const TDataOffset& l, const TDataOffset& r) {
            return TDataOffset::FromEncoded(l.Position_ + r.Position_);
        }

        friend TDataOffset operator-(const TDataOffset& l, const TDataOffset& r) {
            return TDataOffset::FromEncoded(l.Position_ - r.Position_);
        }

        TDataOffset& operator+=(const TDataOffset& other) {
            return *this = *this + other;
        }

        TDataOffset& operator-=(const TDataOffset& other) {
            return *this = *this - other;
        }

        friend bool operator==(const TDataOffset& l, const TDataOffset& r) {
            return l.Position_ == r.Position_;
        }

        friend bool operator!=(const TDataOffset& l, const TDataOffset& r) {
            return l.Position_ != r.Position_;
        }

        friend bool operator<(const TDataOffset& l, const TDataOffset& r) {
            return l.Position_ < r.Position_;
        }

        friend bool operator<=(const TDataOffset& l, const TDataOffset& r) {
            return l.Position_ <= r.Position_;
        }

        friend bool operator>(const TDataOffset& l, const TDataOffset& r) {
            return l.Position_ > r.Position_;
        }

        friend bool operator>=(const TDataOffset& l, const TDataOffset& r) {
            return l.Position_ >= r.Position_;
        }

    private:
        ui64 Position_;
    };

    class TDataOffsetVectorizer {
    public:
        enum {
            TupleSize = 2
        };

        template <class Slice>
        static void Scatter(const TDataOffset& data, Slice&& slice) {
            TUi64Vectorizer::Scatter(data.ToEncoded(), slice);
        }

        template <class Slice>
        static void Gather(Slice&& slice, TDataOffset* data) {
            ui64 tmp;
            TUi64Vectorizer::Gather(slice, &tmp);
            *data = TDataOffset::FromEncoded(tmp);
        }
    };

    class TDataOffsetSerializer {
    public:
        enum {
            MaxSize = TUi64VarintSerializer::MaxSize
        };

        static size_t Serialize(const TDataOffset& data, ui8* output) {
            return TUi64VarintSerializer::Serialize(data.ToEncoded(), output);
        }

        static size_t Deserialize(const ui8* input, TDataOffset* data) {
            ui64 encoded;
            size_t size = TUi64VarintSerializer::Deserialize(input, &encoded);
            *data = TDataOffset::FromEncoded(encoded);

            return size;
        }
    };

}
