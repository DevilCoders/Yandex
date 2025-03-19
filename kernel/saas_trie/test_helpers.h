#pragma once

#include "abstract_trie.h"

#include <library/cpp/testing/unittest/gtest.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

namespace NSaasTrie {
    class TComplexKey;

    namespace NTesting {
        struct ITrieTestContext {
            virtual ~ITrieTestContext() = default;
            virtual void Write(const TVector<std::pair<TString, ui64>>& data) = 0;
            virtual TAtomicSharedPtr<ITrieStorageReader> GetReader() = 0;
        };

        void TestTrieIterators(ITrieTestContext& context,
                               TVector<std::pair<TString, ui64>> testData,
                               const TString& emptyPrefix,
                               const TString& prefix,
                               TVector<std::pair<TString, ui64>> prefixData);

        void TestComplexKeyIterator(ITrieTestContext& context,
                                    TVector<std::pair<TString, ui64>> testData,
                                    const TComplexKey& key,
                                    TVector<std::pair<TString, ui64>> expectedData,
                                    bool lastDimensionUnique = false);

        THolder<ITrieStorageIterator> CreateFakeTrieIterator(const TVector<std::pair<TString, ui64>>& sequence);
        THolder<ITrieStorageIterator> CreateFakeTrieIterator(const TVector<std::pair<TString, ui64>>& sequence, size_t from, size_t to);
        THolder<ITrieStorageIterator> CreateFakeTrieIterator();  // create empty iterator
    }
}
