#include "fasttrie.h"

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/digest/md5/md5.h>
#include <util/string/reverse.h>
#include <util/string/split.h>
#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/charset/unidata.h>

#include <algorithm>

namespace NTrieTest {
    struct TWideTraits {
        typedef wchar16 CharType;

        enum {
            Size = 256,
        };

        static size_t Index(CharType c) {
            return static_cast<unsigned char>(c);
        }

        static bool Equal(const CharType* lhs, const CharType* rhs, size_t l) {
            return std::equal(lhs, lhs + l, rhs);
        }
    };

    struct TWideCITraits: public TWideTraits {
        static size_t Index(CharType c) {
            return static_cast<unsigned char>(::ToLower(c));
        }

        static bool Equal(const CharType* lhs, const CharType* rhs, size_t l) {
            for (size_t i = 0; i < l; ++i) {
                if (::ToLower(lhs[i]) != ::ToLower(rhs[i]))
                    return false;
            }
            return true;
        }
    };

    template <typename TTrie, typename TResult>
    TString Build(TTrie& trie, const typename TTrie::CharType* data, typename TTrie::CharType sep = '\n') {
        TResult result;
        typedef typename TTrie::CharType CharType;
        typedef TBasicStringBuf<CharType> StringBufType;
        TVector<StringBufType> tmp;
        StringSplitter(data, data + std::char_traits<CharType>::length(data)).Split(sep).AddTo(&tmp);

        for (size_t i = 0; i < tmp.size(); ++i) {
            result.Update(&trie.At(tmp[i]), sizeof(typename TTrie::DataType));
            trie[tmp[i]] = i;
        }
        return result.ToString();
    }

    template <typename TTrie, typename TResult>
    TString Test(const TTrie& trie, const typename TTrie::CharType* data, typename TTrie::CharType sep = '\n') {
        TResult result;
        typedef typename TTrie::CharType CharType;
        typedef TBasicStringBuf<CharType> StringBufType;
        TVector<StringBufType> tmp;
        StringSplitter(data, data + std::char_traits<CharType>::length(data)).Split(sep).AddTo(&tmp);

        for (size_t i = 0; i < tmp.size(); ++i) {
            std::pair<size_t, bool> p = trie.FindPath(tmp[i]);
            ui64 first = (ui64)p.first;
            result.Update(&first, sizeof(ui64));
            result.Update((bool*)&p.second, sizeof(bool));
            result.Update(&trie.At(tmp[i]), sizeof(typename TTrie::DataType));
        }
        return result.ToString();
    }
}

struct TResultMD5 {
    MD5 Summ;

    template <typename T>
    void Update(const T* p, size_t l) {
        Summ.Update((const ui8*)p, l);
    }

    TString ToString() {
        char out_buf[26];
        Summ.End_b64(out_buf);
        return out_buf;
    }
};

template <typename TTrie>
void TestFindByPrefix(TString (*Modifier)(const TString&)) {
    {
        TTrie trie;
        TString line;

        line = "shalt not covet";
        line = Modifier(line);
        trie.Insert(line.data(), line.data() + (line.size()), 10);

        line = "thou shalt not covet";
        line = Modifier(line);
        trie.Insert(line.data(), line.data() + (line.size()), 11);

        line = "not covet";
        line = Modifier(line);
        trie.Insert(line.data(), line.data() + (line.size()), 12);

        const int* resPtr = nullptr;

        line = "unrelated";
        line = Modifier(line);
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_EQUAL(resPtr, nullptr);

        line = "covet";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_EQUAL(resPtr, nullptr);

        line = "not covet";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_UNEQUAL(resPtr, nullptr);
        UNIT_ASSERT_VALUES_EQUAL(*resPtr, 12);

        line = "why not covet";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_UNEQUAL(resPtr, nullptr);
        UNIT_ASSERT_VALUES_EQUAL(*resPtr, 12);

        line = "so thou shalt not covet";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_UNEQUAL(resPtr, nullptr);
        UNIT_ASSERT_VALUES_EQUAL(*resPtr, 11);

        line = "";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_EQUAL(resPtr, nullptr);
    }
    {
        TTrie trie;
        TString line;
        const int* resPtr = nullptr;

        line = "";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_EQUAL(resPtr, nullptr);

        line = "anyone here?";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_EQUAL(resPtr, nullptr);

        line = "shalt not covet";
        line = Modifier(line);
        trie.Insert(line.data(), line.data() + (line.size()), 9);
        trie.Insert(line.data(), line.data() + (line.size()), 10);

        line = "unrelated";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_EQUAL(resPtr, nullptr);

        line = "covet";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_EQUAL(resPtr, nullptr);

        line = "shalt not covet";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_UNEQUAL(resPtr, nullptr);
        UNIT_ASSERT_VALUES_EQUAL(*resPtr, 10);

        line = "and oh thou shalt not covet";
        line = Modifier(line);
        resPtr = trie.FindByPrefix(line.data(), line.data() + (line.size()));
        UNIT_ASSERT_VALUES_UNEQUAL(resPtr, nullptr);
        UNIT_ASSERT_VALUES_EQUAL(*resPtr, 10);
    }
}

inline TString ModifyEqual(const TString& src) {
    return src;
}

inline TString ModifyReverse(const TString& src) {
    TString res = src;
    ReverseInPlace(res);
    return res;
}

struct TDebugResult: public TResultMD5 {
    template <typename T>
    void Update(const T* p, size_t l) {
        TResultMD5::Update(p, l);
        Cerr << *p << " ";
    }

    TString ToString() {
        TString res = TResultMD5::ToString();
        Cerr << res << Endl;
        return res;
    }
};

static const char* data =
    "a\n"
    "ab\n"
    "ac\n"
    "bac\n"
    "bad\n"
    "abcd\n"
    "abcd";

static const char* dataCI =
    "a\n"
    "ab\n"
    "AC\n"
    "bac\n"
    "bAd\n"
    "aBcD\n"
    "Abcd";

Y_UNIT_TEST_SUITE(TestTrie) {
    Y_UNIT_TEST(BuildChar) {
        typedef TFastTrie<ui64, TTrieTraits, NTrieDirections::TForward<char, const char*>> TrieType;
        TrieType trie;
        TString build(NTrieTest::Build<TrieType, TResultMD5>(trie, data));
        TString test(NTrieTest::Test<TrieType, TResultMD5>(trie, data));
        UNIT_ASSERT_VALUES_EQUAL(build, "aD4XcMexvwJgnzym56DYFg==");
        UNIT_ASSERT_VALUES_EQUAL(test, "Ovb4YHAXTsgDHyjQj+8LMw==");
    }

    Y_UNIT_TEST(BuildWide) {
        TUtf16String wide(UTF8ToWide(data));
        typedef TFastTrie<ui64, NTrieTest::TWideTraits, NTrieDirections::TForward<wchar16, const wchar16*>> TrieType;
        TrieType trie;
        TString build(NTrieTest::Build<TrieType, TResultMD5>(trie, wide.data()));
        TString test(NTrieTest::Test<TrieType, TResultMD5>(trie, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "aD4XcMexvwJgnzym56DYFg==");
        UNIT_ASSERT_VALUES_EQUAL(test, "Ovb4YHAXTsgDHyjQj+8LMw==");
    }

    Y_UNIT_TEST(BuildCIChar) {
        typedef TFastTrie<ui64, TCITextTrieTraits, NTrieDirections::TForward<char, const char*>> TrieType;
        TrieType trie;
        TString build(NTrieTest::Build<TrieType, TResultMD5>(trie, dataCI));
        TString test(NTrieTest::Test<TrieType, TResultMD5>(trie, dataCI));
        UNIT_ASSERT_VALUES_EQUAL(build, "aD4XcMexvwJgnzym56DYFg==");
        UNIT_ASSERT_VALUES_EQUAL(test, "Ovb4YHAXTsgDHyjQj+8LMw==");
    }

    Y_UNIT_TEST(BuildCIWide) {
        TUtf16String wide(UTF8ToWide(dataCI));
        typedef TFastTrie<ui64, NTrieTest::TWideCITraits, NTrieDirections::TForward<wchar16, const wchar16*>> TrieType;
        TrieType trie;
        TString build(NTrieTest::Build<TrieType, TResultMD5>(trie, wide.data()));
        TString test(NTrieTest::Test<TrieType, TResultMD5>(trie, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "aD4XcMexvwJgnzym56DYFg==");
        UNIT_ASSERT_VALUES_EQUAL(test, "Ovb4YHAXTsgDHyjQj+8LMw==");
    }

    Y_UNIT_TEST(CopyChar) {
        typedef TFastTrie<ui64, TTrieTraits, NTrieDirections::TForward<char, const char*>> TrieType;
        TrieType trie;
        TString build(NTrieTest::Build<TrieType, TResultMD5>(trie, data));
        TString test(NTrieTest::Test<TrieType, TResultMD5>(trie, data));
        UNIT_ASSERT_VALUES_EQUAL(build, "aD4XcMexvwJgnzym56DYFg==");
        UNIT_ASSERT_VALUES_EQUAL(test, "Ovb4YHAXTsgDHyjQj+8LMw==");
        TrieType copy(trie);
        TString testCopy(NTrieTest::Test<TrieType, TResultMD5>(copy, data));
        UNIT_ASSERT_VALUES_EQUAL(testCopy, "Ovb4YHAXTsgDHyjQj+8LMw==");
    }

    Y_UNIT_TEST(CopyWide) {
        typedef TFastTrie<ui64, NTrieTest::TWideTraits, NTrieDirections::TForward<wchar16, const wchar16*>> TrieType;
        TUtf16String wide(UTF8ToWide(data));
        TrieType trie;
        TString build(NTrieTest::Build<TrieType, TResultMD5>(trie, wide.data()));
        TString test(NTrieTest::Test<TrieType, TResultMD5>(trie, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(build, "aD4XcMexvwJgnzym56DYFg==");
        UNIT_ASSERT_VALUES_EQUAL(test, "Ovb4YHAXTsgDHyjQj+8LMw==");
        TrieType copy(trie);
        TString testCopy(NTrieTest::Test<TrieType, TResultMD5>(copy, wide.data()));
        UNIT_ASSERT_VALUES_EQUAL(testCopy, "Ovb4YHAXTsgDHyjQj+8LMw==");
    }

    Y_UNIT_TEST(FindByPrefix) {
        TestFindByPrefix<TFastTrie<int, TTrieTraits, NTrieDirections::TBackward<char, const char*>>>(ModifyEqual);
        TestFindByPrefix<TFastTrie<int, TTrieTraits, NTrieDirections::TForward<char, const char*>>>(ModifyReverse);
    }

} // TEST_SUITE
