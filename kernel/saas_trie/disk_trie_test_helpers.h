#pragma once

#include "disk_io.h"
#include "disk_trie.h"
#include "disk_trie_builder.h"

#include "test_helpers.h"

#include <kernel/saas_trie/test_utils/fake_disk_trie.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>

#include <functional>

namespace NSaasTrie {
    namespace NTesting {
        struct TDiskTrieTestContext : ITrieTestContext {
            const TString TRIE_PATH = "test.trie";
            TFakeDisk Disk;

            void Write(const TVector<std::pair<TString, ui64>>& data) override {
                TTestTrieIterator iterator{data};
                BuildTrieFromIterator(TRIE_PATH, Disk, iterator);
            }

            TAtomicSharedPtr<ITrieStorageReader> GetReader() override {
                return OpenDiskTrie(TRIE_PATH, Disk);
            }
        };
    }
}
