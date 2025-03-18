#pragma once

#include <util/generic/vector.h>
#include <util/system/yassert.h>
#include <util/string/ascii.h>

template <typename TContainer, typename TChar>
static inline void EncodeAscii(const TChar* source, const size_t length, TContainer* dest) {
    static const char szCodes[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV"; // WXYZ

    Y_ASSERT(dest);

    dest->clear();
    dest->reserve(length * 8 / 5 + 2);

    size_t n = 0;
    ssize_t nBits = 0;

    for (size_t i = 0; i < length; ++i) {
        ui8 ch = ui8(source[i]);
        n |= (size_t(ch) << nBits);
        nBits += 8;

        while (nBits > 5) {
            dest->push_back(szCodes[n & 31]);
            n >>= 5;
            nBits -= 5;
        }
    }

    while (nBits > 0) {
        dest->push_back(szCodes[n & 31]);
        n >>= 5;
        nBits -= 5;
    }
}

template <typename TContainer, typename TDestContainer>
static inline void EncodeAscii(const TContainer& source, TDestContainer* dest) {
    EncodeAscii(source.begin(), source.size(), dest);
}

template <typename TContainer, typename TDestContainer>
bool DecodeAscii(const TContainer& source, TDestContainer* dest) {
    Y_ASSERT(dest);

    dest->clear();
    dest->reserve(source.size() * 5 / 8 + 2);

    size_t n = 0;
    size_t nBits = 0;

    for (auto ch : source) {
        unsigned char c = (unsigned char)(AsciiToUpper(ch));

        size_t nVal = 0;
        if (c >= '0' && c <= '9')
            nVal = c - '0';
        else if (c >= 'A' && c <= 'V')
            nVal = 10 + (c - 'A');
        else
            return false;

        n |= nVal << nBits;
        nBits += 5;
        while (nBits >= 8) {
            dest->push_back(n & 0xff);
            n >>= 8;
            nBits -= 8;
        }
    }

    return true;
}
