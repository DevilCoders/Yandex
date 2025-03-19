#include "memory_trie.h"
#include "test_helpers.h"

#include <kernel/saas_trie/idl/saas_trie.pb.h>
#include <kernel/saas_trie/test_utils/test_utils.h>

namespace NSaasTrie {
    namespace NTesting {
        TEST(TTrieComponentSuite, MemoryTrie) {
            auto trie = CreateMemoryTrie();

            EXPECT_EQ(trie->GetSize(), 0);

            ExpectedNotToHave(*trie, "color");

            trie->Put("color", 7);
            EXPECT_EQ(trie->GetSize(), 1);
            ExpectedToHave(*trie, "color", 7);

            trie->Put("color", 11);
            ExpectedToHave(*trie, "color", 11);

            trie->Put("size", 20);
            EXPECT_EQ(trie->GetSize(), 2);
            ExpectedToHave(*trie, "size", 20);

            trie->DeleteIfEqual("size", 30);
            EXPECT_EQ(trie->GetSize(), 2);
            ExpectedToHave(*trie, "size", 20);

            trie->DeleteIfEqual("size", 20);
            EXPECT_EQ(trie->GetSize(), 1);
            ExpectedNotToHave(*trie, "size");

            trie->Delete("col");
            EXPECT_EQ(trie->GetSize(), 1);
            ExpectedToHave(*trie, "color", 11);

            trie->Delete("color");
            EXPECT_EQ(trie->GetSize(), 0);
            ExpectedNotToHave(*trie, "color");
        }

        struct TMemoryTrieTestContext : ITrieTestContext {
            TAtomicSharedPtr<ITrieStorage> Trie;

            TMemoryTrieTestContext() {
                Trie = CreateMemoryTrie();
            }
            void Write(const TVector<std::pair<TString, ui64>>& data) override {
                for (auto& item : data) {
                    Trie->Put(item.first, item.second);
                }
            }
            TAtomicSharedPtr<ITrieStorageReader> GetReader() override {
                return Trie;
            }
        };

        TEST(TTrieComponentSuite, MemoryTrieIterator) {
            TMemoryTrieTestContext context;

            TestTrieIterators(
                context,
                {
                    {"key1", 100},
                    {"color", 200},
                    {"color/red", 1},
                    {"color/green", 2},
                    {"color/blue", 3}
                },
                "size",
                "color/",
                {
                    {"red", 1},
                    {"green", 2},
                    {"blue", 3}
                }
            );
        }

        TEST(TTrieComponentSuite, MemoryTrieRemoveByValues) {
            auto trie = CreateMemoryTrie();
            trie->Put("red", 10);
            trie->Put("green", 200);
            trie->Put("blue", 3);
            trie->Put("orange", 42);

            auto removed = RemoveValuesFromTrie(*trie, {10, 200, 3, 40});
            EXPECT_EQ(removed, 3);
            EXPECT_EQ(trie->GetSize(), 1);
            ExpectedToHave(*trie, "orange", 42);
        }

        TEST(TTrieComponentSuite, MemoryTrieComplexKeyIterator) {
            TMemoryTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix");
            auto set1 = key.AddAllRealms();
            set1->SetName("delimiter");
            set1->AddKey("/");
            auto set2 = key.AddAllRealms();
            set2->SetName("color");
            set2->AddKey("red");
            set2->AddKey("green");
            auto set4 = key.AddAllRealms();
            set4->SetName("fruit");
            set4->AddKey("apple");
            set4->AddKey("orange");
            key.AddKeyRealms("delimiter");
            key.AddKeyRealms("color");
            key.AddKeyRealms("delimiter");
            key.AddKeyRealms("fruit");

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tprefix/green/apple", 3},
                    {"0\tprefix/green/plum", 4},
                    {"0\tprefix/blue/stone", 5},
                    {"0\tprefix/blue/orange", 6},
                    {"0\tprefix/green/orange", 7}
                },
                key,
                {
                    {"0\tprefix/green/apple", 3},
                    {"0\tprefix/green/orange", 7}
                }
            );
        }

        TEST(TTrieComponentSuite, MemoryTrieComplexKeyIteratorCrash) {
            TMemoryTrieTestContext context;
            TComplexKey key;
            auto set2 = key.AddAllRealms();
            set2->SetName("color");
            set2->AddKey("red/");
            auto set4 = key.AddAllRealms();
            set4->SetName("fruit");
            set4->AddKey("apple");
            set4->AddKey("orange");
            key.AddKeyRealms("color");
            key.AddKeyRealms("fruit");

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tgreen/apple", 3},
                    {"0\tgreen/plum", 4},
                    {"0\tblue/stone", 5},
                    {"0\tblue/orange", 6},
                    {"0\tgreen/orange", 7}
                },
                key,
                {}
            );
        }

        TEST(TTrieComponentSuite, MemoryTrieComplexKeyIteratorTrivial) {
            TMemoryTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix/green/orange");

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tprefix/green/orange", 7}
                },
                key,
                {
                    {"0\tprefix/green/orange", 7}
                }
            );
        }

        TEST(TTrieComponentSuite, MemoryTrieComplexKeyIteratorTrivialEmpty) {
            TMemoryTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix");

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tprefix/green/orange", 7}
                },
                key,
                {}
            );
        }

        TEST(TTrieComponentSuite, MemoryTrieComplexKeyIteratorSingleDimension) {
            TMemoryTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix/");
            auto set2 = key.AddAllRealms();
            set2->SetName("color");
            set2->AddKey("red");
            set2->AddKey("green");
            key.AddKeyRealms("color");

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tprefix/green", 3},
                    {"0\tprefix/green/plum", 4},
                    {"0\tprefix/blue/stone", 5},
                    {"0\tprefix/blue/orange", 6},
                    {"0\tprefix/green/orange", 7}
                },
                key,
                {
                    {"0\tprefix/green", 3}
                }
            );
        }
    }
}
