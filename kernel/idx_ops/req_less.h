#pragma once

#include "types.h"

#include <util/generic/strbuf.h>
#include <util/generic/bitops.h>
#include <util/string/cast.h>

namespace NIdxOps {
    //! Compare request ids as ints.
    inline bool IntLess(const TRequestId left, const TRequestId right) {
        return left < right;
    }

    /*! Compare request ids as strings.
     * Should be used when input files are sorted in mapreduce.
     *
     * @note TString/Sprintf are very slow, so we use stack buffers instead.
     * ToString converting to buffer is fast (see junk/mvel/tests/sprintf-speed benchmark)
     */
    inline bool StrLessSlow(const ui32 left, const ui32 right) {
        static_assert(sizeof(int) <= 8, "expect sizeof(int) <= 8"); // increase buffer size below if int becomes longer
        const size_t bufSize = 24;
        char ls[bufSize];
        TStringBuf lsb(ls, ToString(left, ls, bufSize));
        char rs[bufSize];
        TStringBuf rsb(rs, ToString(right, rs, bufSize));
        return lsb < rsb;
    }

    /**
      * Fast version of StrLess (by and42@)
      **/
    inline bool StrLess(const TRequestId left, const TRequestId right) {
        static const unsigned char maxDecimalDigit[] = {
             0,  0,  0,  0,  1,  1,  1,  2,
             2,  2,  3,  3,  3,  3,  4,  4,
             4,  5,  5,  5,  6,  6,  6,  6,
             7,  7,  7,  8,  8,  8,  9,  9,
             9,  9, 10, 10, 10, 11, 11, 11,
            12, 12, 12, 12, 13, 13, 13, 14,
            14, 14, 15, 15, 15, 15, 16, 16,
            16, 17, 17, 17, 18, 18, 18, 18,
            19 };

        static const ui64 e10[] = {
            1ull,
            10ull,
            100ull,
            1000ull,
            10000ull,
            100000ull,
            1000000ull,
            10000000ull,
            100000000ull,
            1000000000ull,
            10000000000ull,
            100000000000ull,
            1000000000000ull,
            10000000000000ull,
            100000000000000ull,
            1000000000000000ull,
            10000000000000000ull,
            100000000000000000ull,
            1000000000000000000ull,
            10000000000000000000ull };

        size_t lDecPos = left ? maxDecimalDigit[GetValueBitCount(left)] : 0;
        size_t rDecPos = right ? maxDecimalDigit[GetValueBitCount(right)] : 0;
        int lDigit = left / e10[lDecPos];
        int rDigit = right / e10[rDecPos];

        // skip lead zero if any
        if (!lDigit && lDecPos)
            lDigit = left / e10[--lDecPos];

        if (!rDigit && rDecPos)
            rDigit = right / e10[--rDecPos];

        while (1) {
            if (lDigit != rDigit)
                return lDigit < rDigit;

            if (lDecPos && rDecPos) {
                lDigit = left / e10[--lDecPos];
                rDigit = right / e10[--rDecPos];
            } else {
                return lDecPos < rDecPos;
            }
        }
    }

    //! Comparator for request ids
    typedef bool (*TIntComparator)(const TRequestId, const TRequestId);
} // namespace NIdxOps
