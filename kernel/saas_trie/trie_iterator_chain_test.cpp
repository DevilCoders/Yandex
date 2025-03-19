#include "trie_iterator_chain.h"

#include "test_helpers.h"

#include <kernel/saas_trie/test_utils/test_utils.h>

#include <library/cpp/testing/unittest/gtest.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NSaasTrie {
    namespace {
        using NTesting::CreateFakeTrieIterator;

        TVector<std::pair<TString, ui64>> GetTestData() {
            return {
                {"key1", 1},
                {"key2", 2},
                {"key3", 3},
                {"key4", 4},
                {"key5", 5}
            };
        }

        TEST(TrieIteratorChainSuite, ChainPair) {
            auto testData = GetTestData();
            NTesting::CheckIterator(CreateFakeTrieIterator(testData, 0u, testData.size()), testData);
            auto chain = CreateTrieIteratorChain(
                CreateFakeTrieIterator(testData, 0u, testData.size() / 2),
                CreateFakeTrieIterator(testData, testData.size() / 2, testData.size())
            );
            NTesting::CheckIterator(std::move(chain), testData);
        }

        TEST(TrieIteratorChainSuite, ChainWithEmptyIterators) {
            auto testData = GetTestData();
            auto chain = CreateTrieIteratorChain(
                CreateFakeTrieIterator(),
                CreateFakeTrieIterator(testData, 0u, testData.size() / 2),
                CreateFakeTrieIterator(),
                CreateFakeTrieIterator(testData, testData.size() / 2, testData.size()),
                CreateFakeTrieIterator()
            );
            NTesting::CheckIterator(std::move(chain), testData);
        }

        TEST(TrieIteratorChainSuite, VectorPair) {
            auto testData = GetTestData();
            NTesting::CheckIterator(CreateFakeTrieIterator(testData, 0u, testData.size()), testData);
            TTrieIteratorVector iterators;
            iterators.emplace_back(CreateFakeTrieIterator(testData, 0u, testData.size() / 2));
            iterators.emplace_back(CreateFakeTrieIterator(testData, testData.size() / 2, testData.size()));
            auto chain = DecorateTrieIteratorVector(std::move(iterators));
            NTesting::CheckIterator(std::move(chain), testData);
        }

        TEST(TrieIteratorChainSuite, VectorWithEmptyIterators) {
            auto testData = GetTestData();
            TTrieIteratorVector iterators;
            iterators.emplace_back(CreateFakeTrieIterator());
            iterators.emplace_back(CreateFakeTrieIterator(testData, 0u, testData.size() / 2));
            iterators.emplace_back(CreateFakeTrieIterator());
            iterators.emplace_back(CreateFakeTrieIterator(testData, testData.size() / 2, testData.size()));
            iterators.emplace_back(CreateFakeTrieIterator());
            auto chain = DecorateTrieIteratorVector(std::move(iterators));
            NTesting::CheckIterator(std::move(chain), testData);
        }
    }
}
