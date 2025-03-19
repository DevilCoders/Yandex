#pragma once

#include <util/system/defaults.h>
#include <util/system/unaligned_mem.h>
#include <util/system/valgrind.h>

#include "group_id.gen"
#include <library/cpp/wordpos/wordpos.h>

#if defined(__has_feature)
#   if __has_feature(address_sanitizer)
#       include <sanitizer/asan_interface.h>
#   endif
#endif

int CodeWordPos(const SUPERLONG &prev, const SUPERLONG &cur, char *pBuf);

#if defined(_little_endian_) && !defined(_must_align2_)
#   define IC_GET16(x, pBuf) (x) = ReadUnaligned<ui16>(pBuf)
#else
#   define IC_GET16(x, pBuf) do { \
        const unsigned char *p = (const unsigned char*)(pBuf);\
        ui16 __icRes = *p++;\
        __icRes |= ((ui16)*p++) << 8;\
        (x) = __icRes; \
      } while (0)
#endif
#if defined(_little_endian_) && !defined(_must_align4_)
#   define IC_GET32(x, pBuf) (x) = ReadUnaligned<ui32>(pBuf)
#else
#   define IC_GET32(x, pBuf) do { \
        const unsigned char *p = (const unsigned char*)(pBuf);\
        ui32 __icRes = *p++;\
        __icRes |= ((ui32)*p++) << 8;\
        __icRes |= ((ui32)*p++) << 16;\
        __icRes |= ((ui32)*p++) << 24;\
        (x) = __icRes; \
      } while (0)
#endif

extern const ui64 invCodeMaskAndOffset[];
extern const ui64 invCodeByteMask[];
const int MEMORY_PAGE_SIZE = 4096;
Y_FORCE_INLINE const char *DecodeWordPos(const SUPERLONG prev, SUPERLONG *__restrict pRes, const char * pBuf) {
    unsigned char nGroup = *pBuf++;
    SUPERLONG *pMaskAndOffset = ((SUPERLONG*)invCodeMaskAndOffset) + nGroup + nGroup;
    SUPERLONG bits;
    bits = (prev & pMaskAndOffset[0]) + pMaskAndOffset[1];

    if (Y_LIKELY(nGroup < N_INVCODE_GROUP2_START)) {
        if (Y_LIKELY(nGroup < N_INVCODE_GROUP1_START)) {
            // group 0 (0,0,0,0,0)
        } else {
            // group 1 (0,6,2,0,0)
            unsigned long nRes = (unsigned char)*pBuf++;
            unsigned long nAdd = 0;
            nAdd += (nRes & 3) << WORD_LEVEL_Shift;
            nAdd += (nRes & (63 << 2)) << (BREAK_LEVEL_Shift - 2);
            bits += nAdd;
        }
    } else {
        if (nGroup < N_INVCODE_GROUP4_START) {
            if (nGroup < N_INVCODE_GROUP3_START) {
                // group 2 (3,8,4,0,1)
                unsigned long nRes;
                IC_GET16(nRes, pBuf);
                unsigned long nAdd = 0;
                nAdd += (nRes & 1);
                nAdd += (nRes & (15 << 1)) << (WORD_LEVEL_Shift - 1);
                nAdd += (nRes & (255 << 5)) << (BREAK_LEVEL_Shift - 5);
                nAdd += (nRes & (7 << 13)) << (DOC_LEVEL_Shift - 13);
                bits += nAdd;
                pBuf += 2;
            } else {
                // group 3 (4,8,6,2,4)
                unsigned long nRes;
                IC_GET32(nRes, pBuf - 1);
                nRes >>= 8;
                unsigned long nAdd = 0;
                assert(DOC_LEVEL_Shift + 4 <= 32);
                nAdd += (nRes & (15 | (3 << 4) | (63 << 6) | (255 << 12)));
                nAdd += (nRes & (15 << 20)) << (DOC_LEVEL_Shift - 20);
                bits += nAdd;
                pBuf += 3;
            }
        } else {
            if (nGroup < N_INVCODE_GROUP_REST) {
                // Group 4 (12,8,6,2,4)
                unsigned long nRes;
                IC_GET32(nRes, pBuf);
                bits += (nRes & (15 | (3 << 4) | (63 << 6) | (255 << 12)));
                bits += ((SUPERLONG)(nRes & (4095ul << 20))) << (DOC_LEVEL_Shift - 20);
                pBuf += 4;
            } else {
                // Group rest
                SUPERLONG diff = 0;
                int nBytes = nGroup - N_INVCODE_GROUP_REST;
#if defined(_little_endian_) && !defined(_must_align8_)
                if (((pBuf - ((const char*)nullptr)) & (MEMORY_PAGE_SIZE - 1)) < MEMORY_PAGE_SIZE - 9) {
                    // page fault is impossible
                    const unsigned int v = VALGRIND_MAKE_READABLE(pBuf, 8);
                    VALGRIND_DISCARD(v);
#if defined(ASAN_UNPOISON_MEMORY_REGION)
                    ASAN_UNPOISON_MEMORY_REGION(pBuf, 8);
#endif
                    diff = ReadUnaligned<SUPERLONG>(pBuf);
                    diff &= ((SUPERLONG*)invCodeByteMask)[nBytes];
                    pBuf += nBytes;
                } else {
                    const char *pEnd = pBuf + nBytes;
                    for (char *pDst = (char*)&diff; pBuf < pEnd;)
                        *pDst++ = *pBuf++;
                }
#else
                const char *pEnd = pBuf + nBytes;
                int nShift = 0;
                for (; pBuf < pEnd;) {
                    SUPERLONG nByte = (unsigned char)*pBuf++;
                    diff |= nByte << nShift;
                    nShift += 8;
                }
#endif
                bits = prev + diff;
            }
        }
    }
    *pRes = bits;
    return pBuf;
}
