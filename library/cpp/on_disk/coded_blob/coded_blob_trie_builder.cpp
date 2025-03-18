#include "coded_blob_trie_builder.h"

namespace NCodedBlob {
    TCodedBlobTrieBuilder::TCodedBlobTrieBuilder(bool unsorted, IAllocator* alloc)
        : KeysBuilder(unsorted, alloc)
    {
    }

    void TCodedBlobTrieBuilder::Init(IOutputStream* out, NCodecs::TCodecPtr trainedcodec) {
        Out = out;
        NUtils::DoWriteHeader(Out, CODED_BLOB_TRIE_MAGIC, CODED_BLOB_TRIE_VERSION);
        DataBuilder.Init(out, trainedcodec);
    }

    void TCodedBlobTrieBuilder::Finish() {
        DataBuilder.Finish();
        KeysBuilder.WriteWithFooter(Out);
    }

}
