#include "test_helpers.h"

#include "trie_complex_key_iterator.h"

#include <kernel/saas_trie/idl/saas_trie.pb.h>
#include <kernel/saas_trie/test_utils/test_utils.h>

#include <util/generic/algorithm.h>

namespace NSaasTrie {
    namespace NTesting {
        void TestTrieIterators(ITrieTestContext& context,
                               TVector<std::pair<TString, ui64>> testData,
                               const TString& emptyPrefix,
                               const TString& prefix,
                               TVector<std::pair<TString, ui64>> prefixData) {
            context.Write(testData);

            Sort(testData, [](auto& a, auto& b) {
                return a.first < b.first;
            });
            Sort(prefixData, [](auto& a, auto& b) {
                return a.first < b.first;
            });

            auto trie = context.GetReader();
            ASSERT_TRUE(!!trie);

            CheckIterator(trie->CreateIterator(), testData);
            CheckIterator(trie->CreatePrefixIterator(emptyPrefix), {});
            CheckIterator(trie->CreatePrefixIterator(prefix), prefixData);

            CheckSubtree(trie->GetSubTree(""), testData);
            CheckSubtree(trie->GetSubTree(emptyPrefix), {});
            CheckSubtree(trie->GetSubTree(prefix), prefixData);

            TString halfPrefix1{prefix.data(), prefix.size() / 2};
            TString halfPrefix2{prefix.data() + halfPrefix1.size(), prefix.size() - halfPrefix1.size()};
            CheckSubtree(trie->GetSubTree(halfPrefix1)->GetSubTree(halfPrefix2), prefixData);
        }

        void TestComplexKeyIterator(ITrieTestContext& context,
                                    TVector<std::pair<TString, ui64>> testData,
                                    const TComplexKey& key,
                                    TVector<std::pair<TString, ui64>> expectedData,
                                    bool lastDimensionUnique) {
            context.Write(testData);
            auto trie = context.GetReader();
            ASSERT_TRUE(!!trie);

            bool needSort = !(lastDimensionUnique || key.HasLastRealmUnique() && key.GetLastRealmUnique() ||
                    key.HasUrlMaskPrefix());
            auto prepKey = PreprocessComplexKey(key, needSort, {});

            if (needSort) {
                Sort(expectedData, [](auto& a, auto& b) {
                    return a.first < b.first;
                });
            }
            CheckIterator(CreateTrieComplexKeyIterator(*trie, prepKey), expectedData);
        }


        struct TFakeIterator : ITrieStorageIterator {
            TFakeIterator() = default;

            TFakeIterator(const TVector<std::pair<TString, ui64>>& sequence, size_t from, size_t to) {
                Data.assign(sequence.begin() + from, sequence.begin() + to);
            }

            bool AtEnd() const override {
                return Current >= Data.size();
            }
            TString GetKey() const override {
                return Data[Current].first;
            }
            ui64 GetValue() const override {
                return Data[Current].second;
            }
            bool Next() override {
                return ++Current < Data.size();
            }

        private:
            TVector<std::pair<TString, ui64>> Data;
            size_t Current = 0;
        };

        THolder<ITrieStorageIterator> CreateFakeTrieIterator(const TVector<std::pair<TString, ui64>>& sequence, size_t from, size_t to) {
            return MakeHolder<TFakeIterator>(sequence, from, to);
        }

        THolder<ITrieStorageIterator> CreateFakeTrieIterator(const TVector<std::pair<TString, ui64>>& sequence) {
            return MakeHolder<TFakeIterator>(sequence, 0u, sequence.size());
        }

        THolder<ITrieStorageIterator> CreateFakeTrieIterator() {
            return MakeHolder<TFakeIterator>();
        }
    }
}
