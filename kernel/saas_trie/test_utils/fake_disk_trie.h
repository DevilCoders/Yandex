#pragma once

#include <kernel/saas_trie/abstract_trie.h>

#include <kernel/saas_trie/disk_io.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSaasTrie {
    namespace NTesting {
        using TFakeDiskTrieContext = std::pair<
            TAtomicSharedPtr<ITrieStorage>,
            THolder<IDiskIO>
        >;

        struct TFakeDisk : IDiskIO {
            mutable THashMap<TString, TString> FakeFiles;

            TBlob Map(const TString& path) const override final;
            THolder<IOutputStream> CreateWriter(const TString& path) const override final;
        };

        struct TTestTrieIterator : ITrieStorageIterator {
            explicit TTestTrieIterator(const TVector<std::pair<TString, ui64>>& data);

            bool AtEnd() const override final;
            TString GetKey() const override final;
            ui64 GetValue() const override final;
            bool Next() override final;

        private:
            TVector<std::pair<TString, ui64>>::const_iterator Iterator, End;
        };

        TFakeDiskTrieContext CreateFakeDiskTrie(const TVector<std::pair<TString, ui64>>& testData);
    }
}
