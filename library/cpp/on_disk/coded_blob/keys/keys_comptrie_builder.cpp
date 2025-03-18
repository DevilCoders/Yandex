#include "keys_comptrie_builder.h"

namespace NCodedBlob {
    TCompactTrieKeysBuilder::TCompactTrieKeysBuilder(bool unsorted, IAllocator* alloc)
        : KeysBuilder(unsorted ? CTBF_NONE : CTBF_PREFIX_GROUPED, TPrimaryKeys::TBuilder::TPacker(), alloc)
    {
    }

    void TCompactTrieKeysBuilder::WriteWithFooter(IOutputStream* out) {
        TBuffer result;
        result.Reserve(2 * KeysSize + (sizeof(ui64) + 1) * KeysCount);
        TBufferOutput bout(result);

        ::Save(&bout, EmptyKeyOffset);
        size_t headersize = result.Size();

        KeysBuilder.SaveAndDestroy(bout);
        result.ShrinkToFit();

        {
            TBufferOutput tmp(result.Size() * 1.1);
            tmp.Write(result.Data(), headersize);
            CompactTrieMakeFastLayout(tmp, result.data() + headersize, result.size() - headersize, false, TPrimaryKeys::TBuilder::TPacker());
            result.Swap(tmp.Buffer());
        }

        NUtils::DoWriteWithFooter(out, result.data(), result.size());
    }

}
