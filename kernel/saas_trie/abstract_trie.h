#pragma once

#include <util/generic/strbuf.h>
#include <util/system/types.h>

namespace NSaasTrie {
    struct ITrieStorageIterator {
        virtual ~ITrieStorageIterator() = default;

        virtual bool AtEnd() const = 0;
        virtual TString GetKey() const = 0;
        virtual ui64 GetValue() const = 0;
        virtual bool Next() = 0;
    };

    struct ITrieStorageReader {
        virtual ~ITrieStorageReader() = default;

        virtual bool Get(TStringBuf key, ui64& value) const = 0;
        virtual ui64 GetSize() const = 0;
        virtual bool IsEmpty() const = 0;

        // iterators and subtrees must not last longer than their parent tries
        virtual THolder<ITrieStorageIterator> CreateIterator() const = 0;
        virtual THolder<ITrieStorageIterator> CreatePrefixIterator(TStringBuf prefix) const = 0;
        virtual THolder<ITrieStorageReader> GetSubTree(TStringBuf prefix) const = 0;
    };

    struct ITrieStorageWriter {
        virtual ~ITrieStorageWriter() = default;

        virtual bool IsReadOnly() const = 0;
        virtual bool Put(TStringBuf key, ui64 value) = 0;
        virtual bool Delete(TStringBuf key) = 0;
        virtual bool DeleteIfEqual(TStringBuf key, ui64 value) = 0;
        virtual void Discard() = 0;
    };

    struct ITrieStorage : ITrieStorageReader, ITrieStorageWriter {
    };
}
