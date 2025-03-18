#include "chacha.h"

#include <util/stream/output.h>


namespace NHypocrisy {


namespace {


ui32 Rotl(ui32 operand, ui32 shift) {
    return (operand << shift) | (operand >> (32 - shift));
}


void PerformQuarterRound(ui32* dst, size_t a, size_t b, size_t c, size_t d) {
    dst[a] += dst[b];
    dst[d] = Rotl(dst[d] ^ dst[a], 16);

    dst[c] += dst[d];
    dst[b] = Rotl(dst[b] ^ dst[c], 12);

    dst[a] += dst[b];
    dst[d] = Rotl(dst[d] ^ dst[a], 8);

    dst[c] += dst[d];
    dst[b] = Rotl(dst[b] ^ dst[c], 7);
}


} // namespace


void PerformChaChaDoubleRound(ui32* dst) {
    PerformQuarterRound(dst, 0, 4, 8, 12);
    PerformQuarterRound(dst, 1, 5, 9, 13);
    PerformQuarterRound(dst, 2, 6, 10, 14);
    PerformQuarterRound(dst, 3, 7, 11, 15);

    PerformQuarterRound(dst, 0, 5, 10, 15);
    PerformQuarterRound(dst, 1, 6, 11, 12);
    PerformQuarterRound(dst, 2, 7, 8, 13);
    PerformQuarterRound(dst, 3, 4, 9, 14);
}


} // namespace NHypocrisy
