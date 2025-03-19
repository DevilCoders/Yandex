#include "inv_code.h"
#include <assert.h>
#include <memory.h>

#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif

#include "group_sel.gen"
static SUPERLONG nBorders[8] = {
    1, 0x100, 0x10000, 0x1000000, 0x100000000ll, 0x10000000000ll, 0x1000000000000ll, 0x100000000000000ll
};
const ui64 invCodeByteMask[] = {
    ULL(0), ULL(0xff), ULL(0xffff), ULL(0xffffff), ULL(0xffffffff), ULL(0xffffffffff), ULL(0xffffffffffff), ULL(0xffffffffffffff), ULL(0xffffffffffffffff)
};
int CodeWordPos(const SUPERLONG &prev, const SUPERLONG &cur, char *pBuf)
{
    // detect group
    SUPERLONG nDocDelta = (cur >> (23+NFORM_LEVEL_Bits)) - (prev >> (23+NFORM_LEVEL_Bits));
    int nFields[5], nDeltas[5];
    nFields[0] = (int)(nDocDelta);
    nFields[1] = (int)((cur >> BREAK_LEVEL_Shift) & BREAK_LEVEL_Max);
    nFields[2] = (int)((cur >> WORD_LEVEL_Shift) & WORD_LEVEL_Max);
    nFields[3] = (int)((cur >> RELEV_LEVEL_Shift) & RELEV_LEVEL_Max);
    nFields[4] = (int)((cur >> NFORM_LEVEL_Shift) & NFORM_LEVEL_Max);

    nDeltas[0] = nFields[0];
    nDeltas[1] = nFields[1] - (int)((prev >> BREAK_LEVEL_Shift) & BREAK_LEVEL_Max);
    nDeltas[2] = nFields[2] - (int)((prev >> WORD_LEVEL_Shift) & WORD_LEVEL_Max);
    nDeltas[3] = nFields[3] - (int)((prev >> RELEV_LEVEL_Shift) & RELEV_LEVEL_Max);
    nDeltas[4] = nFields[4] - (int)((prev >> NFORM_LEVEL_Shift) & NFORM_LEVEL_Max);

    unsigned char nGroup = 0xff;
    int nCodeLength = 8;
    if (nDocDelta < 0x40000000) {
        GETGROUP0dddd(nGroup, nCodeLength, nFields[0], nDeltas[1], nDeltas[2], nDeltas[3], nDeltas[4]);
        GETGROUP0dFdd(nGroup, nCodeLength, nFields[0], nDeltas[1], nFields[2], nDeltas[3], nDeltas[4]);
        GETGROUP1dddd(nGroup, nCodeLength, nFields[0], nDeltas[1], nDeltas[2], nDeltas[3], nDeltas[4]);
        GETGROUP1dFdd(nGroup, nCodeLength, nFields[0], nDeltas[1], nFields[2], nDeltas[3], nDeltas[4]);
        GETGROUP1FFdd(nGroup, nCodeLength, nFields[0], nFields[1], nFields[2], nDeltas[3], nDeltas[4]);
        GETGROUP2FFdd(nGroup, nCodeLength, nFields[0], nFields[1], nFields[2], nDeltas[3], nDeltas[4]);
        GETGROUP3FFFF(nGroup, nCodeLength, nFields[0], nFields[1], nFields[2], nFields[3], nFields[4]);
        GETGROUP4FFFF(nGroup, nCodeLength, nFields[0], nFields[1], nFields[2], nFields[3], nFields[4]);
    }

    SUPERLONG diff = cur - prev;
    {
        ASSERT(N_INVCODE_GROUP_REST <= 256 - 9);
        int nDiffCodeLength = 8;
        if (diff >= 0) {
            while (nDiffCodeLength > 0 && diff < nBorders[nDiffCodeLength - 1])
                --nDiffCodeLength;
        } else {
            nDiffCodeLength = 8;
        }
        if (nGroup == 0xff || nDiffCodeLength < nCodeLength) {
            nGroup = (unsigned char)(N_INVCODE_GROUP_REST + nDiffCodeLength);
        }
    }
    char *pBufStart = pBuf;
    *pBuf++ = nGroup;

    SUPERLONG *pMaskAndOffset = ((SUPERLONG*)invCodeMaskAndOffset) + ((int)nGroup) * 2;
    SUPERLONG bits = cur - (prev & pMaskAndOffset[0]) - pMaskAndOffset[1];

    if (nGroup < N_INVCODE_GROUP2_START) {
        if (nGroup < N_INVCODE_GROUP1_START) {
            // group 0 (0,0,0,0,0)
        } else {
            // group 1 (0,6,2,0,0)
            unsigned char nRes;
            nRes = (unsigned char)((bits >> WORD_LEVEL_Shift) & 3);
            nRes |= ((bits >> (BREAK_LEVEL_Shift - 2)) & (63<<2));
            *pBuf++ = nRes;
        }
    } else {
        if (nGroup < N_INVCODE_GROUP4_START) {
            if (nGroup < N_INVCODE_GROUP3_START) {
                // group 2 (3,8,4,0,1)
                ui16 nRes = (ui16)(bits & 1);
                nRes |= (bits >> (WORD_LEVEL_Shift - 1)) & (15 << 1);
                nRes |= (bits >> (BREAK_LEVEL_Shift - 5)) & (255 << 5);
                nRes |= (bits >> (DOC_LEVEL_Shift - 13)) & (7 << 13);
                *pBuf++ = (unsigned char)nRes;
                *pBuf++ = (unsigned char)(nRes>>8);
            } else {
                // group 3 (4,8,6,2,4)
                ui32 nRes = (ui32)(bits & (15 | (3 << 4) | (63 << 6) | (255 << 12)));
                nRes |= (bits >> (DOC_LEVEL_Shift - 20)) & (15 << 20);
                *pBuf++ = (unsigned char)nRes;
                *pBuf++ = (unsigned char)(nRes>>8);
                *pBuf++ = (unsigned char)(nRes>>16);
            }
        } else {
            if (nGroup < N_INVCODE_GROUP_REST) {
                // Group 4 (12,8,6,2,4)
                ui32 nRes = (ui32)(bits & (15 | (3 << 4) | (63 << 6) | (255 << 12)));
                nRes |= (bits >> (DOC_LEVEL_Shift - 20)) & (4095 << 20);
#if defined(_little_endian_) && !defined(_must_align4_)
                WriteUnaligned<ui32>(pBuf, nRes);
                pBuf += 4;
#else
                *pBuf++ = (unsigned char)nRes;
                *pBuf++ = (unsigned char)(nRes>>8);
                *pBuf++ = (unsigned char)(nRes>>16);
                *pBuf++ = (unsigned char)(nRes>>24);
#endif
            } else {
                // Group rest
                int nBytes = nGroup - N_INVCODE_GROUP_REST;
#if defined(_little_endian_)
                memcpy(pBuf, &diff, nBytes);
                pBuf += nBytes;
#else
                SUPERLONG diffOut = diff;
                for (int i = 0; i < nBytes; ++i) {
                    *pBuf++ = (unsigned char)diffOut;
                    diffOut >>= 8;
                }
#endif
            }
        }
    }
#ifndef NDEBUG
    {
        SUPERLONG test;
        DecodeWordPos(prev, &test, pBufStart);
        ASSERT(test == cur);
    }
#endif
    return (int)(pBuf - pBufStart);
}
