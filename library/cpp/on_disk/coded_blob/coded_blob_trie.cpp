#include "coded_blob_trie.h"

namespace NCodedBlob {
    static_assert(sizeof(CODED_BLOB_TRIE_MAGIC) == NUtils::CODED_BLOB_MAGIC_SIZE + 1,
                  "expect sizeof(CODED_BLOB_TRIE_MAGIC) == NUtils::CODED_BLOB_MAGIC_SIZE + 1");

    void TCodedBlobTrie::Init(TBlob b0, ELoadMode loadmode) {
        TotalSize = b0.Size();

        TBlob b = b0;
        NUtils::DoReadHeader(b, CODED_BLOB_TRIE_MAGIC, CODED_BLOB_TRIE_VERSION);

        LoadMode = loadmode;

        TBlob body = NUtils::DoSkipBody(b);
        Data.Init(body);

        if (LM_RAM_MMAP == LoadMode) {
            NUtils::MoveToRam(b);
        }

        Keys.Init(b);

        if (LM_RAM_MMAP == LoadMode) {
            // only body will be mmapped
            NUtils::SetRandomAccessed(body);
        } else if (LM_MMAP == LoadMode) {
            // all the trie is going to be mmapped
            NUtils::SetRandomAccessed(b0);
        }
    }

    TString TCodedBlobTrie::ReportLoadMode() const {
        switch (LoadMode) {
            default:
                return "???";
            case LM_RAM:
                return "ram";
            case LM_RAM_MMAP:
                return "ram+mmap";
            case LM_MMAP:
                return "mmap";
        }
    }

    ui64 TCodedBlobTrie::ApproxLoadedSize(bool ramOnly) const {
        ui64 res = 0;
        res += (ramOnly && LM_MMAP == LoadMode) ? 0 : Keys.Size();
        res += (ramOnly && LM_RAM != LoadMode) ? 0 : Data.Size();
        return res;
    }

    ui64 TCodedBlobTrie::PredictLoadSize(TBlob b, ELoadMode loadmode) {
        switch (loadmode) {
            default:
            case LM_RAM:
                return b.Size();
            case LM_RAM_MMAP:
                // adjusts b
                NUtils::DoSkipBody(b);
                return b.Size();
            case LM_MMAP:
                return 0;
        }
    }

}
