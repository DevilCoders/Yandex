#pragma once

#include "trie_common.h"

struct TLeafCounter {
    typedef ui32 TValue;

    static inline size_t HowMuch(size_t offset, size_t size) {
        (void)offset; // shut up, warnings
        return (size >= 2) ? sizeof(ui8) + (size - 1) * sizeof(TValue) + ALIGN_ADD_PTR_X(ALIGN_OF(TValue), offset + sizeof(ui8)) : 0;
    }

    static inline const ui8* GetEnd(const ui8* const pos, size_t size) {
        if (size < 2)
            return pos;

        if (!*pos)
            return pos + sizeof(ui8) + (size - 1) * sizeof(TValue) + ALIGN_ADD_PTR_X(ALIGN_OF(TValue), pos + sizeof(ui8));
        else {
            ui8 dataSize = NTriePrivate::GetSize(*pos);
            Y_ASSERT(dataSize % 8 == 0); // not implemented bits precision
            dataSize /= 8;
            Y_ASSERT(dataSize > 0);

            return pos + sizeof(ui8) + (size - 1) * dataSize;
        }
    }

    static inline void Init(ui8* pos, size_t size) {
        memset(pos, 0, HowMuch((size_t)pos, size));
    }

    static inline NTriePrivate::TEnds Optimize(ui8* pos, size_t size) {
        if (size >= 2) {
            Y_ASSERT(*pos == 0); // optimize only once
            return NTriePrivate::OptimizeArray<TValue>(pos, pos + 1, size - 1);
        } else {
            return NTriePrivate::TEnds(pos, pos);
        }
    }

    static inline NTriePrivate::TEnds Move(ui8* dst, const ui8* src, size_t size) {
        const ui8* const end = GetEnd(src, size);
        memmove(dst, src, end - src);
        return NTriePrivate::TEnds(end, dst + (end - src));
    }

    static inline size_t Search(const ui8* pos, size_t size, TValue what) {
        Y_ASSERT(*pos);
        const ui8 type = NTriePrivate::GetType(*pos), bits = NTriePrivate::GetSize(*pos);
        pos += sizeof(ui8);
        size_t first = 0, count = size - 1;
        while (count > 0) {
            const size_t step = count / 2;
            const size_t idx = first + step;
            const TValue val = NTriePrivate::Decode<TValue>(type, bits, pos, idx);
            if (!(what < val)) {
                first = idx + 1;
                count -= step + 1;
            } else
                count = step;
        }
        return first;
    }
    static const ui32 RecordSig = 1;

    static TValue PostProcessTrie(ui8* const data, ui8* pos);
};
