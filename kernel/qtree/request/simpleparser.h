#pragma once

#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

template <typename TCharType>
class TSimpleParser {
public:
    typedef TSimpleParser<TCharType> TSelf;

    TSimpleParser(const TCharType* s)
        : p(s)
    {
    }
    TSelf& Check(TCharType c) {
        if (*p != c)
            ythrow yexception() << "wrong character";
        ++p;
        return *this;
    }
    template <size_t N>
    TSelf& Check(const TCharType(&arr)[N]) {
        return String(arr, N - 1);
    }
    TSelf& Copy(size_t n, TCharType* val) {
        std::char_traits<TCharType>::copy(val, p, n);
        p += n;
        return *this;
    }
    TSelf& Copy(size_t n, TCharType* val, TCharType cMin, TCharType cMax) {
        if (!InRange(p, p + n, cMin, cMax))
            throw yexception();
        return Copy(n, val);
    }
    template <typename T>
    TSelf& Parse(size_t n, T& val) {
        val = FromString<T>(p, n);
        p += n;
        return *this;
    }
private:
    bool InRange(const TCharType* first, const TCharType* last, TCharType cMin, TCharType cMax) {
        for (; first != last; ++first) {
            if (*first < cMin || *first > cMax)
                return false;
        }
        return true;
    }
private:
    const TCharType* p;
};
