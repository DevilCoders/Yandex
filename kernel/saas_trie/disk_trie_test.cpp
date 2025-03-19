#include "disk_trie_test_helpers.h"

#include <kernel/saas_trie/idl/trie_key.h>
#include <kernel/saas_trie/test_utils/test_utils.h>

namespace NSaasTrie {
    namespace NTesting {
        TEST(TTrieComponentSuite, DiskTrie) {
            TDiskTrieTestContext context;
            context.Write({
                {"key1", 1},
                {"key2", 22}
            });

            auto trie = context.GetReader();

            EXPECT_EQ(trie->GetSize(), 2);
            ExpectedToHave(*trie, "key1", 1);
            ExpectedToHave(*trie, "key2", 22);
        }

        TEST(TTrieComponentSuite, DiskTrieIterator) {
            TDiskTrieTestContext context;
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

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIterator) {
            TDiskTrieTestContext context;
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

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorCrash) {
            TDiskTrieTestContext context;
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

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorTrivial) {
            TDiskTrieTestContext context;
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

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorTrivialEmpty) {
            TDiskTrieTestContext context;
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

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorSingleDimension) {
            TDiskTrieTestContext context;
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

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorMultipleDimensions) {
            TDiskTrieTestContext context;
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
                    {"0\tprefix/red/apple", 3},
                    {"0\tprefix/green/plum", 4},
                    {"0\tprefix/blue/stone", 5},
                    {"0\tprefix/blue/orange", 6},
                    {"0\tprefix/red/orange", 7}
                },
                key,
                {
                    {"0\tprefix/red/apple", 3},
                    {"0\tprefix/red/orange", 7}
                }
            );
        }

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorPrefixRealms) {
            TDiskTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix");
            auto set1 = key.AddAllRealms();
            set1->SetName("delimiter");
            set1->AddKey("/");
            auto set2 = key.AddAllRealms();
            set2->SetName("color");
            set2->AddKey("red");
            set2->AddKey("green");
            set2->AddKey("blue");
            auto set4 = key.AddAllRealms();
            set4->SetName("fruit");
            set4->AddKey("apple");
            set4->AddKey("orange");
            key.AddKeyRealms("delimiter");
            key.AddKeyRealms("color");
            key.AddKeyRealms("delimiter");
            key.AddKeyRealms("fruit");
            WriteRealmPrefixSet(key, {1});

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tprefix/red/apple", 3},
                    {"0\tprefix/green/plum", 4},
                    {"0\tprefix/blue/stone", 5},
                    {"0\tprefix/blue/orange", 6},
                    {"0\tprefix/red/orange", 7},
                    {"0\tprefix/green/apple", 8},
                    {"0\tprefix/green", 9},
                    {"0\tprefix/blue", 10}
                },
                key,
                {
                    {"0\tprefix/blue", 10},
                    {"0\tprefix/blue/orange", 6},
                    {"0\tprefix/green", 9},
                    {"0\tprefix/green/apple", 8},
                    {"0\tprefix/red/apple", 3},
                    {"0\tprefix/red/orange", 7}
                },
                false
            );
        }

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorPrefixRealms2) {
            TDiskTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix");
            auto set1 = key.AddAllRealms();
            set1->SetName("delimiter");
            set1->AddKey("/");
            set1->AddKey(":");
            auto set2 = key.AddAllRealms();
            set2->SetName("color");
            set2->AddKey("blue");
            set2->AddKey("green");
            set2->AddKey("red");
            auto set4 = key.AddAllRealms();
            set4->SetName("fruit");
            set4->AddKey("apple");
            set4->AddKey("orange");
            key.AddKeyRealms("delimiter");
            key.AddKeyRealms("color");
            key.AddKeyRealms("delimiter");
            key.AddKeyRealms("fruit");
            WriteRealmPrefixSet(key, {1});

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tprefix/red/apple", 3},
                    {"0\tprefix/green/plum", 4},
                    {"0\tprefix/red/orange", 7},
                    {"0\tprefix/green/apple", 8},
                    {"0\tprefix/green", 9},
                    {"0\tprefix:", 10},
                    {"0\tprefix/green:", 11}
                },
                key,
                {
                    {"0\tprefix/green", 9},
                    {"0\tprefix/green/apple", 8},
                    {"0\tprefix/red/apple", 3},
                    {"0\tprefix/red/orange", 7}
                },
                false
            );
        }

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorPrefixRealms3) {
            TDiskTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix");
            auto set1 = key.AddAllRealms();
            set1->SetName("delimiter");
            set1->AddKey("/");
            set1->AddKey(":");
            auto set2 = key.AddAllRealms();
            set2->SetName("color");
            set2->AddKey("blue");
            set2->AddKey("green");
            set2->AddKey("red");
            auto set4 = key.AddAllRealms();
            set4->SetName("fruit");
            set4->AddKey("apple");
            set4->AddKey("orange");
            key.AddKeyRealms("delimiter");
            key.AddKeyRealms("color");
            key.AddKeyRealms("fruit");
            WriteRealmPrefixSet(key, {1});

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tprefix/red/apple", 3},
                    {"0\tprefix/green/plum", 4},
                    {"0\tprefix/red/orange", 7},
                    {"0\tprefix/green/apple", 8},
                    {"0\tprefix/green", 9},
                    {"0\tprefix:", 10},
                    {"0\tprefix/green:", 11},
                    {"0\tprefix/redapple", 12},
                    {"0\tprefix/red", 13}
                },
                key,
                {
                    {"0\tprefix/green", 9},
                    {"0\tprefix/red", 13},
                    {"0\tprefix/redapple", 12}
                },
                false
            );
        }

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorPrefixRealms4) {
            TDiskTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix");
            auto set2 = key.AddAllRealms();
            set2->SetName("color");
            set2->AddKey("/blue");
            set2->AddKey("/green");
            set2->AddKey("/red");
            auto set4 = key.AddAllRealms();
            set4->SetName("fruit");
            set4->AddKey("/apple");
            set4->AddKey("/orange");
            key.AddKeyRealms("color");
            key.AddKeyRealms("fruit");
            WriteRealmPrefixSet(key, {0});

            TestComplexKeyIterator(
                context,
                {
                    {"0\tprefix/blue/stone", 3},
                    {"0\tprefix/green/stone", 4},
                    {"0\tprefix/green", 9},
                    {"0\tprefix:", 10},
                    {"0\tprefix/green:", 11},
                    {"0\tprefix/red/apple", 12},
                    {"0\tprefix/red", 13}
                },
                key,
                {
                    {"0\tprefix/green", 9},
                    {"0\tprefix/red", 13},
                    {"0\tprefix/red/apple", 12}
                },
                false
            );
        }

        TEST(TTrieComponentSuite, DiskTrieComplexKeyIteratorPrefixRealmsEmpty) {
            TDiskTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix");
            auto set1 = key.AddAllRealms();
            set1->SetName("delimiter");
            set1->AddKey("/");
            auto set2 = key.AddAllRealms();
            set2->SetName("color");
            set2->AddKey("red");
            set2->AddKey("green");
            set2->AddKey("blue");
            auto set4 = key.AddAllRealms();
            set4->SetName("fruit");
            set4->AddKey("apple");
            set4->AddKey("orange");
            key.AddKeyRealms("delimiter");
            key.AddKeyRealms("color");
            key.AddKeyRealms("delimiter");
            key.AddKeyRealms("fruit");
            WriteRealmPrefixSet(key, {1});

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tpref/red/apple", 3}
                },
                key,
                {
                },
                false
            );
        }

        TEST(TTrieComponentSuite, DiskTrieSingleRealmIteratorEmpty) {
            TDiskTrieTestContext context;
            TComplexKey key;
            key.SetMainKey("prefix");
            auto set2 = key.AddAllRealms();
            set2->SetName("color");
            set2->AddKey("red");
            set2->AddKey("green");
            set2->AddKey("blue");
            key.AddKeyRealms("color");
            WriteRealmPrefixSet(key, {1});

            TestComplexKeyIterator(
                context,
                {
                    {"0000", 1},
                    {"wrong_key", 2},
                    {"0\tpref/red/apple", 3}
                },
                key,
                {
                },
                false
            );
        }
    }
}
