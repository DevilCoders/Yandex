#pragma once

#include <util/generic/fwd.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

template<class CharType>
class Y_PACKED TBasicCompactStringBuf {
public:
    TBasicCompactStringBuf() : BeginPtr(nullptr), SizeVal(0) {}
    TBasicCompactStringBuf(const CharType* begin, ui8 size) : BeginPtr(begin), SizeVal(size) {}
    TBasicCompactStringBuf(const CharType* begin, const CharType* end) : BeginPtr(begin) {
        size_t size = end - begin;
        Y_ENSURE(size <= std::numeric_limits<ui8>::max(), "String is too long to store in TBasicCompactStringBuf");
        SizeVal = size;
    }
    TBasicCompactStringBuf(const TBasicString<CharType>& str) : BeginPtr(str.data()) {
        size_t size = str.size();
        Y_ENSURE(size <= std::numeric_limits<ui8>::max(), "String is too long to store in TBasicCompactStringBuf");
        SizeVal = size;
    }
    TBasicCompactStringBuf(const TBasicStringBuf<CharType> str) : BeginPtr(str.data()) {
        size_t size = str.size();
        Y_ENSURE(size <= std::numeric_limits<ui8>::max(), "String is too long to store in TBasicCompactStringBuf");
        SizeVal = size;
    }

    bool operator==(const TBasicCompactStringBuf<CharType> other) const {
        return (TBasicStringBuf<CharType>(BeginPtr, SizeVal) == TBasicStringBuf<CharType>(other.BeginPtr, other.SizeVal));
    }

    const CharType* data() const {
        return BeginPtr;
    }

    ui8 size() const {
        return SizeVal;
    }

private:
    const CharType* BeginPtr;
    ui8 SizeVal;
};

template<class CharType>
struct THash<TBasicCompactStringBuf<CharType>> {
    static constexpr THash<TBasicStringBuf<CharType>> Hasher{};
    
    inline size_t operator()(const TBasicCompactStringBuf<CharType> x) const {
        return Hasher(TBasicStringBuf<CharType>(x.data(), x.size()));
    }
};

using TCompactWtringBuf = TBasicCompactStringBuf<wchar16>;

struct TUInt24 {
    ui8 data[3] = {};

    TUInt24() {}

    TUInt24(ui32 n) {
        Y_ENSURE((n >> 24) == 0, "Number is too large to store in TUInt24");
        data[0] = n & 0b1111'1111;
        data[1] = (n >> 8) & 0b1111'1111;
        data[2] = (n >> 16) & 0b1111'1111;
    }

    operator ui32() const {
        ui32 res = data[0] | (data[1] << 8) | (data[2] << 16);
        return res;
    }
};
