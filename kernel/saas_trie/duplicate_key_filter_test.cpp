#include "duplicate_key_filter.h"

#include "test_helpers.h"

#include <kernel/saas_trie/test_utils/test_utils.h>

#include <library/cpp/testing/unittest/gtest.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NSaasTrie {
    namespace {
        auto CreateFilteredIterator(TVector<std::pair<TString, ui64>> data) {
            return FilterDuplicateKeys(NTesting::CreateFakeTrieIterator(std::move(data)));
        }

        TEST(DuplicateKeyFilterSuite, MainScenario) {
            auto filteredIterator = CreateFilteredIterator({
                {"key1", 1},
                {"key1", 2},
                {"key2", 3},
                {"key2", 4},
                {"key5", 5}
            });
            NTesting::CheckIterator(std::move(filteredIterator), {
                {"key1", 1},
                {"key2", 3},
                {"key5", 5}
            });
        }

        TEST(DuplicateKeyFilterSuite, NoDuplicates) {
            auto filteredIterator = CreateFilteredIterator({
                {"key1", 1},
                {"key2", 2},
                {"key3", 3},
                {"key4", 4},
                {"key5", 5}
            });
            NTesting::CheckIterator(std::move(filteredIterator), {
                {"key1", 1},
                {"key2", 2},
                {"key3", 3},
                {"key4", 4},
                {"key5", 5}
            });
        }

        TEST(DuplicateKeyFilterSuite, Empty) {
            auto filteredIterator = CreateFilteredIterator({});
            NTesting::CheckIterator(std::move(filteredIterator), {});
        }
    }
}
