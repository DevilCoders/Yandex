#include "test_utils.h"

#include <library/cpp/testing/unittest/gtest.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NSaasTrie {
    namespace NTesting {
        void ExpectedToHave(const ITrieStorageReader& storage, TStringBuf key, ui64 expectedValue) {
            ui64 actualValue = 0;
            EXPECT_TRUE(storage.Get(key, actualValue));
            EXPECT_EQ(actualValue, expectedValue);
        }

        void ExpectedNotToHave(const ITrieStorageReader& storage, TStringBuf key) {
            ui64 dummy;
            EXPECT_FALSE(storage.Get(key, dummy));
        }

        void CheckIterator(THolder<ITrieStorageIterator> iterator, const TVector<std::pair<TString, ui64>>& expected) {
            for (size_t i = 0, last = expected.size(); i < last; ++i) {
                EXPECT_FALSE(iterator->AtEnd());
                EXPECT_EQ(iterator->GetKey(), expected[i].first);
                EXPECT_EQ(iterator->GetValue(), expected[i].second);
                EXPECT_EQ(iterator->Next(), (i + 1 < last));
            }
            EXPECT_TRUE(iterator->AtEnd());
        }

        void CheckSubtree(THolder<ITrieStorageReader> trie, const TVector<std::pair<TString, ui64>>& expected) {
            auto iterator = trie->CreateIterator();
            for (size_t i = 0, last = expected.size(); i < last;) {
                EXPECT_FALSE(iterator->AtEnd());
                EXPECT_EQ(iterator->GetKey(), expected[i].first);
                EXPECT_EQ(iterator->GetValue(), expected[i].second);
                EXPECT_EQ(iterator->Next(), ++i < last);
            }
            EXPECT_TRUE(iterator->AtEnd());
            iterator.Reset();
            for (auto& expectedItem : expected) {
                ExpectedToHave(*trie, expectedItem.first, expectedItem.second);
            }
        }
    }
}
