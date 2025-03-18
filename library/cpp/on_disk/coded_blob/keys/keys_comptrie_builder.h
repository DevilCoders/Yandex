#pragma once

#include "keys_comptrie.h"

#include <library/cpp/containers/comptrie/comptrie_builder.h>

#include <util/memory/alloc.h>

namespace NCodedBlob {
    class TCompactTrieKeysBuilder {
        using TPrimaryKeys = TCompactTrieKeys::TPrimaryKeys;

    public:
        TCompactTrieKeysBuilder(bool unsorted = false, IAllocator* alloc = TDefaultAllocator::Instance());

        void WriteWithFooter(IOutputStream* out);

        template <class TKeysIter>
        void AddOffset(TKeysIter keysBegin, TKeysIter keysEnd, ui64 offset) {
            for (; keysBegin != keysEnd; ++keysBegin) {
                const TStringBuf key = *keysBegin;
                KeysSize += key.size();
                KeysCount += 1;
                if (Y_UNLIKELY(!key)) {
                    if (NUtils::INVALID_OFFSET != EmptyKeyOffset) {
                        ythrow TCodedBlobException() << "duplicate empty key";
                    }

                    EmptyKeyOffset = offset;
                } else {
                    if (!KeysBuilder.Add(key, offset)) {
                        ythrow TCodedBlobException() << "duplicate key " << key;
                    }
                }
            }
        }

        void AddOffset(TStringBuf key, ui64 offset) {
            const TStringBuf* keysBegin = &key;
            const TStringBuf* keysEnd = keysBegin + 1;
            AddOffset(keysBegin, keysEnd, offset);
        }

        void AddExtraKey(TStringBuf newKey, TStringBuf existingKey) {
            ui64 offset = 0;
            if (KeysBuilder.Find(existingKey, &offset))
                AddOffset(newKey, offset);
            else
                ythrow TCodedBlobException() << "attempt to add extra key " << newKey << " to nonexisting value with key " << existingKey;
        }

    private:
        TPrimaryKeys::TBuilder KeysBuilder;
        ui64 KeysSize = 0;
        ui64 KeysCount = 0;
        ui64 EmptyKeyOffset = NUtils::INVALID_OFFSET;
    };

}
