#pragma once

#include "coded_blob_trie.h"
#include "coded_blob_builder.h"

#include <library/cpp/on_disk/coded_blob/keys/keys_comptrie_builder.h>

#include <util/memory/alloc.h>

namespace NCodedBlob {
    class TCodedBlobTrieBuilder {
    public:
        TCodedBlobTrieBuilder(bool unsorted = false, IAllocator* alloc = TDefaultAllocator::Instance());

        void Init(IOutputStream* out, NCodecs::TCodecPtr trainedcodec);

        void Finish();

        template <class TKeysIter>
        void AddPrepared(TKeysIter keysBegin, TKeysIter keysEnd, TStringBuf prepareddata) {
            const ui64 offset = DataBuilder.AddPrepared(prepareddata);
            KeysBuilder.AddOffset(keysBegin, keysEnd, offset);
        }

        void AddPrepared(TStringBuf key, TStringBuf prepareddata) {
            const TStringBuf* keysBegin = &key;
            const TStringBuf* keysEnd = keysBegin + 1;
            return AddPrepared(keysBegin, keysEnd, prepareddata);
        }

        template <class TKeysIter>
        void AddCompressed(TKeysIter keysBegin, TKeysIter keysEnd, TStringBuf codeddata) {
            AddPrepared(keysBegin, keysEnd, NUtils::PrepareCompressedValueForBlob(codeddata, TmpBuf));
        }

        void AddCompressed(TStringBuf key, TStringBuf codeddata) {
            const TStringBuf* keysBegin = &key;
            const TStringBuf* keysEnd = keysBegin + 1;
            return AddCompressed(keysBegin, keysEnd, codeddata);
        }

        void AddExtraKey(TStringBuf newKey, TStringBuf existingKey) {
            KeysBuilder.AddExtraKey(newKey, existingKey);
        }

    private:
        IOutputStream* Out = nullptr;
        TCodedBlobBuilder DataBuilder;
        TCompactTrieKeysBuilder KeysBuilder;
        TBuffer TmpBuf;
    };

}
