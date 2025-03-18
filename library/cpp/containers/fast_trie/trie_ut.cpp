#include <util/charset/wide.h>
#include <util/charset/unidata.h>
#include <library/cpp/digest/md5/md5.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/string/split.h>
#include <library/cpp/testing/unittest/registar.h>

#include "tree.h"
#include "trie.h"

namespace NTreeTest {
    struct TWideArrayTraits {
        typedef ::wchar16 TKey;
        typedef const ::wchar16* TKeyIter;

        enum {
            Size = 256,
        };

        static bool Initialized(wchar16 key) {
            return key != 0;
        }

        static size_t Index(wchar16 key) {
            return static_cast<unsigned char>(key);
        }
    };
    typedef TBaseArrayTreeTraits<TWideArrayTraits> TWideArrayTreeTraits;

    template <typename TResult, typename TTree, typename TCharType>
    TString Build(TTree& tree, const TCharType* data) {
        TResult result;
        typedef TBasicStringBuf<TCharType> TStringType;
        TVector<TStringType> tmp;
        StringSplitter(data, data + std::char_traits<TCharType>::length(data)).Split(TCharType('\n')).AddTo(&tmp);

        for (size_t i = 0; i < tmp.size(); ++i) {
            result.Update(tmp[i], tree.Path(tmp[i]));
            tree.At(tmp[i]) = i;
        }
        return result.ToString();
    }

    template <typename TResult, typename TTree, typename TCharType>
    TString Test(const TTree& tree, const TCharType* data) {
        TResult result;
        typedef TBasicStringBuf<TCharType> TStringType;
        TVector<TStringType> tmp;
        StringSplitter(data, data + std::char_traits<TCharType>::length(data)).Split(TCharType('\n')).AddTo(&tmp);

        for (size_t i = 0; i < tmp.size(); ++i) {
            result.Update(tmp[i], tree.Path(tmp[i]));
        }
        return result.ToString();
    }
}

struct TResultMD5 {
    MD5 Summ;

    template <typename TCharType>
    void Update(const TBasicStringBuf<TCharType>& key, const std::pair<std::pair<const TCharType*, const TCharType*>, const i64*>& p) {
        i64 res = ((p.first.second - p.first.first) == (key.end() - key.begin()) && p.second) ? *p.second : -1;
        Summ.Update((const ui8*)&res, sizeof(i64));
        Summ.Update((const ui8*)p.first.first, (p.first.second - p.first.first) * sizeof(TCharType));
    }

    TString ToString() {
        char out_buf[26];
        Summ.End_b64(out_buf);
        return out_buf;
    }
};

struct TDebugResult: public TResultMD5 {
    template <typename TCharType>
    void Update(const TBasicStringBuf<TCharType>& key, const std::pair<std::pair<const TCharType*, const TCharType*>, const i64*>& p) {
        TResultMD5::Update(key, p);
        i64 res = ((p.first.second - p.first.first) == (key.end() - key.begin()) && p.second) ? *p.second : -1;
        Cerr << "(" << key << ": " << res << "[" << TBasicStringBuf<TCharType>(p.first.first, p.first.second) << "]) ";
    }

    TString ToString() {
        TString res = TResultMD5::ToString();
        Cerr << res << Endl;
        return res;
    }
};

static const char* data =
    "\n"
    "\n"
    "a\n"
    "a\n"
    "ab\n"
    "ac\n"
    "bac\n"
    "bad\n"
    "bad\n"
    "abcd\n"
    "abcd\n"
    "ab";

/*
static const char* dataCI =
"a\n"
"ab\n"
"AC\n"
"bac\n"
"bAd\n"
"aBcD\n"
"Abcd";
*/

Y_UNIT_TEST_SUITE(TestHashTree) {
    Y_UNIT_TEST(BuildChar) {
        typedef TTreeBase<i64, TBaseHashTreeTraits<char>> TTree;
        TTree tree;
        TString build(NTreeTest::Build<TResultMD5>(tree, data));
        TString test(NTreeTest::Test<TResultMD5>(tree, data));
        UNIT_ASSERT_VALUES_EQUAL(build, "CpYE/uPr9rcSONTw5003KQ==");
        UNIT_ASSERT_VALUES_EQUAL(test, "a3nWN0MhEuetYzWzn7X/jg==");
    }

    Y_UNIT_TEST(BuildWide) {
        TUtf16String wide(UTF8ToWide(data));
        typedef TTreeBase<i64, TBaseHashTreeTraits<wchar16>> TTree;
        TTree tree;
        TString build(NTreeTest::Build<TResultMD5>(tree, wide.data()));
        TString test(NTreeTest::Test<TResultMD5>(tree, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "IqKzh3B1wnlE7giHo9e89A==");
        UNIT_ASSERT_VALUES_EQUAL(test, "66PJMkOBlqOUC+Gp2t9jsg==");
    }

    Y_UNIT_TEST(CopyChar) {
        typedef TTreeBase<i64, TBaseHashTreeTraits<char>> TTree;
        TTree tree;
        TString build(NTreeTest::Build<TResultMD5>(tree, data));
        TString test(NTreeTest::Test<TResultMD5>(tree, data));
        UNIT_ASSERT_VALUES_EQUAL(build, "CpYE/uPr9rcSONTw5003KQ==");
        UNIT_ASSERT_VALUES_EQUAL(test, "a3nWN0MhEuetYzWzn7X/jg==");
        TTree copy(tree);
        TString testCopy(NTreeTest::Test<TResultMD5>(copy, data));
        UNIT_ASSERT_VALUES_EQUAL(testCopy, "a3nWN0MhEuetYzWzn7X/jg==");
    }

    Y_UNIT_TEST(CopyWide) {
        typedef TTreeBase<i64, TBaseHashTreeTraits<wchar16>> TTree;
        TUtf16String wide(UTF8ToWide(data));
        TTree tree;
        TString build(NTreeTest::Build<TResultMD5>(tree, wide.data()));
        TString test(NTreeTest::Test<TResultMD5>(tree, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "IqKzh3B1wnlE7giHo9e89A==");
        UNIT_ASSERT_VALUES_EQUAL(test, "66PJMkOBlqOUC+Gp2t9jsg==");
        TTree copy(tree);
        TString testCopy(NTreeTest::Test<TResultMD5>(copy, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(testCopy, "66PJMkOBlqOUC+Gp2t9jsg==");
    }
} // TestHashTree

Y_UNIT_TEST_SUITE(TestArrayTree) {
    Y_UNIT_TEST(BuildChar) {
        typedef TTreeBase<i64, TCharArrayTreeTraits> TTree;
        TTree tree;
        TString build(NTreeTest::Build<TResultMD5>(tree, data));
        TString test(NTreeTest::Test<TResultMD5>(tree, data));
        UNIT_ASSERT_VALUES_EQUAL(build, "CpYE/uPr9rcSONTw5003KQ==");
        UNIT_ASSERT_VALUES_EQUAL(test, "a3nWN0MhEuetYzWzn7X/jg==");
    }

    Y_UNIT_TEST(BuildWide) {
        TUtf16String wide(UTF8ToWide(data));
        typedef TTreeBase<i64, NTreeTest::TWideArrayTreeTraits> TTree;
        TTree tree;
        TString build(NTreeTest::Build<TResultMD5>(tree, wide.data()));
        TString test(NTreeTest::Test<TResultMD5>(tree, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "IqKzh3B1wnlE7giHo9e89A==");
        UNIT_ASSERT_VALUES_EQUAL(test, "66PJMkOBlqOUC+Gp2t9jsg==");
    }

    Y_UNIT_TEST(CopyChar) {
        typedef TTreeBase<i64, TCharArrayTreeTraits> TTree;
        TTree tree;
        TString build(NTreeTest::Build<TResultMD5>(tree, data));
        TString test(NTreeTest::Test<TResultMD5>(tree, data));
        UNIT_ASSERT_VALUES_EQUAL(build, "CpYE/uPr9rcSONTw5003KQ==");
        UNIT_ASSERT_VALUES_EQUAL(test, "a3nWN0MhEuetYzWzn7X/jg==");
        TTree copy(tree);
        TString testCopy(NTreeTest::Test<TResultMD5>(copy, data));
        UNIT_ASSERT_VALUES_EQUAL(testCopy, "a3nWN0MhEuetYzWzn7X/jg==");
    }

    Y_UNIT_TEST(CopyWide) {
        typedef TTreeBase<i64, NTreeTest::TWideArrayTreeTraits> TTree;
        TUtf16String wide(UTF8ToWide(data));
        TTree tree;
        TString build(NTreeTest::Build<TResultMD5>(tree, wide.data()));
        TString test(NTreeTest::Test<TResultMD5>(tree, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "IqKzh3B1wnlE7giHo9e89A==");
        UNIT_ASSERT_VALUES_EQUAL(test, "66PJMkOBlqOUC+Gp2t9jsg==");
        TTree copy(tree);
        TString testCopy(NTreeTest::Test<TResultMD5>(copy, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(testCopy, "66PJMkOBlqOUC+Gp2t9jsg==");
    }
} // TestHashTree

const char* trieData =
    "0catch.com\n"
    "0pk.ru\n"
    "1bb.ru\n"
    "2bb.ru\n"
    "3bb.ru\n"
    "3dn.ru\n"
    "4bb.ru\n"
    "4u.ru\n"
    "50megs.com\n"
    "50webs.com";

Y_UNIT_TEST_SUITE(TestHashTrie) {
    Y_UNIT_TEST(BuildCharForward) {
        typedef TTreeBase<i64, TBaseHashTrieTraits<char>> TTrie;
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, trieData));
        TString test(NTreeTest::Test<TResultMD5>(trie, trieData));
        UNIT_ASSERT_VALUES_EQUAL(build, "MpzfXEQIjS8dZHZixFUPlw==");
        UNIT_ASSERT_VALUES_EQUAL(test, "GA7kmTASGAGaNLCjQwBQ5Q==");
    }

    Y_UNIT_TEST(BuildCharBackward) {
        typedef TTreeBase<i64, TBaseHashTrieTraits<char, NDirections::TBackward>, NDirections::TBackward> TTrie;
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, trieData));
        TString test(NTreeTest::Test<TResultMD5>(trie, trieData));
        UNIT_ASSERT_VALUES_EQUAL(build, "/R1WURVLnqfzhRIVF0JS5A==");
        UNIT_ASSERT_VALUES_EQUAL(test, "GA7kmTASGAGaNLCjQwBQ5Q==");
    }

    Y_UNIT_TEST(BuildWideForward) {
        typedef TTreeBase<i64, TBaseHashTrieTraits<wchar16>> TTrie;
        TUtf16String wide(UTF8ToWide(trieData));
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, wide.data()));
        TString test(NTreeTest::Test<TResultMD5>(trie, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "MpzfXEQIjS8dZHZixFUPlw==");
        UNIT_ASSERT_VALUES_EQUAL(test, "0z6mx3kls0LU/YYZAG9iog==");
    }

    Y_UNIT_TEST(CopyCharForward) {
        typedef TTreeBase<i64, TBaseHashTrieTraits<char>> TTrie;
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, trieData));
        TString test(NTreeTest::Test<TResultMD5>(trie, trieData));
        UNIT_ASSERT_VALUES_EQUAL(build, "MpzfXEQIjS8dZHZixFUPlw==");
        UNIT_ASSERT_VALUES_EQUAL(test, "GA7kmTASGAGaNLCjQwBQ5Q==");
        TTrie copy(trie);
        TString testCopy(NTreeTest::Test<TResultMD5>(trie, trieData));
        UNIT_ASSERT_VALUES_EQUAL(testCopy, "GA7kmTASGAGaNLCjQwBQ5Q==");
    }

    Y_UNIT_TEST(CopyWideForward) {
        typedef TTreeBase<i64, TBaseHashTrieTraits<wchar16>> TTrie;
        TUtf16String wide(UTF8ToWide(trieData));
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, wide.data()));
        TString test(NTreeTest::Test<TResultMD5>(trie, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "MpzfXEQIjS8dZHZixFUPlw==");
        UNIT_ASSERT_VALUES_EQUAL(test, "0z6mx3kls0LU/YYZAG9iog==");
        TTrie copy(trie);
        TString testCopy(NTreeTest::Test<TResultMD5>(trie, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(test, "0z6mx3kls0LU/YYZAG9iog==");
    }

    Y_UNIT_TEST(EmptySuffix) {
        TString str =
            "abcd\n"
            "abc\n"
            "abc";
        typedef TTreeBase<i64, TBaseHashTrieTraits<char>> TTrie;
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, str.data()));
        TString test(NTreeTest::Test<TResultMD5>(trie, str.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "YKY6VM4c+PbhXTs/n2hZ+w==");
        UNIT_ASSERT_VALUES_EQUAL(test, "npTDphkp15p4zhr6Td+mjQ==");
    }
}

Y_UNIT_TEST_SUITE(TestArrayTrie) {
    Y_UNIT_TEST(BuildCharForward) {
        typedef TTreeBase<i64, TCharArrayTrieTraits> TTrie;
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, trieData));
        TString test(NTreeTest::Test<TResultMD5>(trie, trieData));
        UNIT_ASSERT_VALUES_EQUAL(build, "MpzfXEQIjS8dZHZixFUPlw==");
        UNIT_ASSERT_VALUES_EQUAL(test, "GA7kmTASGAGaNLCjQwBQ5Q==");
    }

    Y_UNIT_TEST(BuildCharBackward) {
        typedef TTreeBase<i64, TBaseArrayTrieTraits<TCharArrayTraits, NDirections::TBackward>, NDirections::TBackward> TTrie;
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, trieData));
        TString test(NTreeTest::Test<TResultMD5>(trie, trieData));
        UNIT_ASSERT_VALUES_EQUAL(build, "/R1WURVLnqfzhRIVF0JS5A==");
        UNIT_ASSERT_VALUES_EQUAL(test, "GA7kmTASGAGaNLCjQwBQ5Q==");
    }

    Y_UNIT_TEST(BuildWideForward) {
        typedef TTreeBase<i64, TBaseArrayTrieTraits<NTreeTest::TWideArrayTraits>> TTrie;
        TUtf16String wide(UTF8ToWide(trieData));
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, wide.data()));
        TString test(NTreeTest::Test<TResultMD5>(trie, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "MpzfXEQIjS8dZHZixFUPlw==");
        UNIT_ASSERT_VALUES_EQUAL(test, "0z6mx3kls0LU/YYZAG9iog==");
    }

    Y_UNIT_TEST(CopyCharForward) {
        typedef TTreeBase<i64, TCharArrayTrieTraits> TTrie;
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, trieData));
        TString test(NTreeTest::Test<TResultMD5>(trie, trieData));
        UNIT_ASSERT_VALUES_EQUAL(build, "MpzfXEQIjS8dZHZixFUPlw==");
        UNIT_ASSERT_VALUES_EQUAL(test, "GA7kmTASGAGaNLCjQwBQ5Q==");
        TTrie copy(trie);
        TString testCopy(NTreeTest::Test<TResultMD5>(trie, trieData));
        UNIT_ASSERT_VALUES_EQUAL(testCopy, "GA7kmTASGAGaNLCjQwBQ5Q==");
    }

    Y_UNIT_TEST(CopyWideForward) {
        typedef TTreeBase<i64, TBaseArrayTrieTraits<NTreeTest::TWideArrayTraits>> TTrie;
        TUtf16String wide(UTF8ToWide(trieData));
        TTrie trie;
        TString build(NTreeTest::Build<TResultMD5>(trie, wide.data()));
        TString test(NTreeTest::Test<TResultMD5>(trie, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "MpzfXEQIjS8dZHZixFUPlw==");
        UNIT_ASSERT_VALUES_EQUAL(test, "0z6mx3kls0LU/YYZAG9iog==");
        TTrie copy(trie);
        TString testCopy(NTreeTest::Test<TResultMD5>(copy, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(test, "0z6mx3kls0LU/YYZAG9iog==");
    }
}

#if 0
struct TTestSuffixTreeData;
using TTestSuffixTreeBase = TTreeBase<TTestSuffixTreeData, TCharArrayTrieTraits, NDirections::TForward>;

struct TTestSuffixTreeData: public TSuffixTreeData<i64, TTestSuffixTreeBase::TNode> {
    TTestSuffixTreeData()
    {
    }

    TTestSuffixTreeData(i64 val, TTestSuffixTreeBase::TNode* node)
        : TSuffixTreeData<i64, TTestSuffixTreeBase::TNode>(val, node)
    {
    }
};

class TTestSuffixTree: public TSuffixTreeImpl<TTestSuffixTreeBase> {
public:
    TTestSuffixTree()
    {
    }

    i64 Update(i64 val, const TString& s) {
        return TSuffixTreeImpl<TTestSuffixTreeBase>::Update(val, s.c_str(), s.end()).Node->Data.Value;
    }
};

Y_UNIT_TEST_SUITE(TestArraySuffixTree) {
    Y_UNIT_TEST(CharForward) {
        TTestSuffixTree tree;
        Cerr << "Result:\t" << tree.Update(0, TString("cacaocacaocacao")) << Endl;
        Cerr << "Result:\t" << tree.Update(1, TString("cacaofoo")) << Endl;
        Cerr << "Result:\t" << tree.Update(2, TString("cacocacaocacao")) << Endl;
    }
}

#endif
