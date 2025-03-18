#include "trie_enumerated.h"

ui32 TLeafCounter::PostProcessTrie(ui8* const data, ui8* pos) {
    ui32 result = 0;
    td_chr l = acr<td_chr>(pos), num = acr<td_chr>(pos);

    if (num) {
        pos += size_td_offs_info + num * size_td_chr;
        pos = ALIGN_PTR_X(size_td_offs, pos);
        td_offs_t* offs = (td_offs_t*)pos;

        result = PostProcessTrie(data, data + offs[0]);

        if (num > 1) {
            pos += num * size_td_offs;
            TLeafCounter::TValue* counts = (TLeafCounter::TValue*)ALIGN_PTR_X(sizeof(TLeafCounter::TValue), pos + sizeof(ui8));

            for (td_chr i = 1; i < num; i++) {
                counts[i - 1] = result;
                result += PostProcessTrie(data, data + offs[i]);
            }
        }
    }

    if (!(l & td_info_flag))
        result++;

    return result;
}
