#include <util/datetime/cputimer.h>
#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/random/random.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/system/tempfile.h>
#include <util/stream/output.h>
#include <util/stream/file.h>

#include <library/cpp/testing/unittest/registar.h>

#include "reader.h"
#include "writer.h"

namespace {
    /// some non-standart container
    template <class O>
    class TSumContainerBuilder {
        O Out_;

    public:
        TSumContainerBuilder()
            : Out_(0)
        {
        }

        void AddOut(const O& o) {
            Out_ += o;
        }

        bool IsEmpty() const {
            return Out_ == 0;
        }

        void SaveContent(IOutputStream* buffer) const {
            WriteBin<O>(buffer, Out_);
        }
    };

    template <class O> /// O must be the fixed-size type
    class TMappedSumOutputContainer {
        const O* Out_;

    public:
        TMappedSumOutputContainer(const char* data)
            : Out_((const O*)data)
        {
        }

        bool IsEmpty() const {
            return ReadUnaligned<O>(Out_) == 0;
        }

        void FillAnswer(TAhoSearchResult<O>& answer, ui32 pos) const {
            if (!IsEmpty())
                answer.push_back(std::make_pair(pos, ReadUnaligned<O>(Out_)));
        }

        size_t CheckData() const {
            return sizeof(O);
        }
    };

    const TString AHO_SAVE_FILE = "trie.bin";

    /// manual test data
    const TString AHO_TEXT = "abacabadacabaca";
    const ui32 AHO_INPUT_CNT = 7;
    const TString AHO_INPUT_DATA[AHO_INPUT_CNT] = {
        "ab",
        "abc",
        "a",
        "acb",
        "aca",
        "a",
        ""};
    const TVector<TString> AHO_INPUT(AHO_INPUT_DATA, AHO_INPUT_DATA + AHO_INPUT_CNT);

    /// jury answer computing

    bool Occurence(const TString& p, const TString& text, ui32 pos) {
        return (p.length() > 0 && p.length() <= pos + 1 && text.substr(pos - p.length() + 1, p.length()) == p);
    }

    template <class O, class C>
    struct TNaiveContainerEmulation {
    };

    template <class O>
    struct TNaiveContainerEmulation<O, TDefaultContainerBuilder<O>> {
        void Do(const TVector<TString>& pattern, const TString& text, TAhoSearchResult<O>& result) {
            for (size_t pos = 0; pos < text.length(); ++pos) {
                for (size_t j = 0; j < pattern.size(); ++j) {
                    if (Occurence(pattern[j], text, pos)) {
                        result.push_back(std::make_pair(pos, j));
                    }
                }
            }
        }
    };

    template <class O>
    struct TNaiveContainerEmulation<O, TSingleContainerBuilder<O>> {
        void Do(const TVector<TString>& pattern, const TString& text, TAhoSearchResult<O>& result) {
            for (size_t pos = 0; pos < text.length(); ++pos) {
                TSet<TString> used;
                for (int j = (int)pattern.size() - 1; j >= 0; --j) {
                    if (used.count(pattern[j]) == 0) {
                        used.insert(pattern[j]);
                        if (Occurence(pattern[j], text, pos)) {
                            result.push_back(std::make_pair(pos, j));
                        }
                    }
                }
            }
        }
    };

    template <class O>
    struct TNaiveContainerEmulation<O, TSumContainerBuilder<O>> {
        void Do(const TVector<TString>& pattern, const TString& text, TAhoSearchResult<O>& result) {
            for (size_t pos = 0; pos < text.length(); ++pos) {
                THashMap<TString, ui32> used;
                for (size_t j = 0; j < pattern.size(); ++j) {
                    if (Occurence(pattern[j], text, pos)) {
                        used[pattern[j]] += j;
                    }
                }
                for (THashMap<TString, ui32>::const_iterator it = used.begin(); it != used.end(); ++it) {
                    if (it->second) {
                        result.push_back(std::make_pair(pos, it->second));
                    }
                }
            }
        }
    };

    template <class O, class C1, class C2>
    TAhoSearchResult<O> NaiveMethod(const TVector<TString>& pattern, const TString& text) {
        TAhoSearchResult<O> result;
        TNaiveContainerEmulation<O, C1>().Do(pattern, text, result);
        return result;
    }

    /// other useful functions

    const TString GetRandomStroka(size_t maxLen) {
        size_t len = RandomNumber(maxLen + 1);
        TString ret;
        for (size_t j = 0; j < len; ++j) {
            ret += char('a' + RandomNumber(2u));
        }
        return ret;
    }

    template <class TIterator>
    void Print(TIterator begin, TIterator end) {
        TIterator it;
        for (it = begin; it != end; ++it) {
            Cout << it->first << " " << it->second << '\t';
        }
        Cout << Endl;
    }

    template <class O>
    bool AnswersAreEqual(const TAhoSearchResult<O>& lhs, const TAhoSearchResult<O>& rhs) {
        typedef TSet<std::pair<ui32, O>> TAnswerSet;
        TAnswerSet s1(lhs.begin(), lhs.end());
        TAnswerSet s2(rhs.begin(), rhs.end());
        return s1 == s2;
    }
}

class TTestMappedAhoCorasick: public TTestBase {
private:
    UNIT_TEST_SUITE(TTestMappedAhoCorasick);
    UNIT_TEST(TestManual);
    UNIT_TEST(TestRandom);
    UNIT_TEST(TestStringValues);
    //UNIT_TEST(TestSpeedSmall);
    // UNIT_TEST(TestSpeed);
    // UNIT_TEST(TestMemory);
    UNIT_TEST_SUITE_END();

    template <class O, class C1, class C2>
    void Test(const TVector<TString>& pattern, const TString& text) {
        TTempFileHandle file(AHO_SAVE_FILE);
        {
            TAhoCorasickBuilder<TString, O, C1> saver;
            for (size_t i = 0; i < pattern.size(); ++i)
                saver.AddString(pattern[i], i);
            TBlob blob = saver.Save();
            TFixedBufferFileOutput fOut(AHO_SAVE_FILE);
            fOut.Write(blob.Data(), blob.Size());
        }
        TAhoSearchResult<O> answer;
        {
            TBlob blob = TBlob::FromFile(AHO_SAVE_FILE);
            TMappedAhoCorasick<TString, O, C2> searcher(blob);
            searcher.CheckData();
            answer = searcher.AhoSearch(text);
        }
        TAhoSearchResult<O> juryAnswer = NaiveMethod<O, C1, C2>(pattern, text);

        UNIT_ASSERT(AnswersAreEqual(juryAnswer, answer));
    }

    template <class O>
    void TestDefault() {
        Test<O, TDefaultContainerBuilder<O>, TMappedDefaultOutputContainer<O>>(AHO_INPUT, AHO_TEXT);
    }

    template <class O>
    void TestSingle() {
        Test<O, TSingleContainerBuilder<O>, TMappedSingleOutputContainer<O>>(AHO_INPUT, AHO_TEXT);
    }

    template <class O>
    void TestSum() {
        Test<O, TSumContainerBuilder<O>, TMappedSumOutputContainer<O>>(AHO_INPUT, AHO_TEXT);
    }

    template <class O>
    void TestRandom() {
        const size_t N = 100;
        TVector<TString> pattern(N);
        for (size_t i = 0; i < N; ++i) {
            pattern[i] = GetRandomStroka(10);
        }
        TString text = GetRandomStroka(1000);

        Test<O, TDefaultContainerBuilder<O>, TMappedDefaultOutputContainer<O>>(pattern, text);
        Test<O, TSingleContainerBuilder<O>, TMappedSingleOutputContainer<O>>(pattern, text);
        Test<O, TSumContainerBuilder<O>, TMappedSumOutputContainer<O>>(pattern, text);
    }

    void TestManual() {
        TestDefault<ui32>();
        TestDefault<int>();
        TestDefault<long long>();

        TestSingle<ui32>();
        TestSingle<long long>();

        TestSum<ui32>();
        TestSum<long long>();
    }

    void TestRandom() {
        TestRandom<ui32>();
        TestRandom<long long>();
    }

    void TestStringValues() {
        static const TString INP[4] = {"abc", "ab", "aba", "aba"};
        static const TString VAL[4] = {"abc", "ab", "aba", "aba2"};
        static const TString TXT = "abaabc";
        typedef std::pair<ui32, TString> TItem;

        TBlob blob;
        {
            TAhoCorasickBuilder<TString, TString> builder;
            for (size_t i = 0; i < 4; ++i)
                builder.AddString(INP[i], VAL[i]);
            blob = builder.Save();
        }
        {
            TMappedAhoCorasick<TString, TString> searcher(blob);
            searcher.CheckData();
            TAhoSearchResult<TString> result = searcher.AhoSearch(TXT);
            UNIT_ASSERT_EQUAL(result.size(), 5);
            UNIT_ASSERT_EQUAL(result[0], TItem(1, "ab"));
            UNIT_ASSERT_EQUAL(result[1], TItem(2, "aba"));
            UNIT_ASSERT_EQUAL(result[2], TItem(2, "aba2"));
            UNIT_ASSERT_EQUAL(result[3], TItem(4, "ab"));
            UNIT_ASSERT_EQUAL(result[4], TItem(5, "abc"));
        }

        {
            TAhoCorasickBuilder<TString, TString, TSingleContainerBuilder<TString>> builder;
            for (size_t i = 0; i < 4; ++i)
                builder.AddString(INP[i], VAL[i]);
            blob = builder.Save();
        }
        {
            TMappedAhoCorasick<TString, TString, TMappedSingleOutputContainer<TString>> searcher(blob);
            searcher.CheckData();
            TAhoSearchResult<TString> result = searcher.AhoSearch(TXT);
            UNIT_ASSERT_EQUAL(result.size(), 4);
            UNIT_ASSERT_EQUAL(result[0], TItem(1, "ab"));
            UNIT_ASSERT_EQUAL(result[1], TItem(2, "aba2"));
            UNIT_ASSERT_EQUAL(result[2], TItem(4, "ab"));
            UNIT_ASSERT_EQUAL(result[3], TItem(5, "abc"));
        }
    }

    void TestSpeed_(size_t ITERATIONS) {
        TBlob blob, blob2;
        {
            TSimpleAhoCorasickBuilder aho;
            TAhoCorasickBuilder<TString, ui32> aho2;
            TVector<TString> v;
            v.push_back("aba");
            v.push_back("abc");
            v.push_back("abd");
            v.push_back("ab123");
            v.push_back("ab1s23");
            v.push_back("abasdas");
            v.push_back("ablkojlkn");
            for (int i = 0; i < 200; ++i)
                v.push_back("l" + ToString(i));

            blob = BuildAho(aho, v.begin(), v.end());
            blob2 = BuildAhoIndex(aho2, v.begin(), v.end());
        }
        TString s("lalafknasjndkjasdjasdokpjkqpjwpedknknabanknsdkfnlksdfkm");
        {
            TSimpleMappedAhoCorasick aho11(blob);
            aho11.CheckData();
            TTimeLogger logger("AhoCorasick");
            for (size_t i = 0; i < ITERATIONS; ++i) {
                TSimpleMappedAhoCorasick aho12(blob);
                aho12.AhoContains(s);
            }
            logger.SetOK();
        }
        {
            TMappedAhoCorasick<TString, ui32> aho21(blob2);
            aho21.CheckData();
            TTimeLogger logger("AhoCorasick2");
            for (size_t i = 0; i < ITERATIONS; ++i) {
                TMappedAhoCorasick<TString, ui32> aho22(blob2);
                aho22.AhoSearch(s);
            }
            logger.SetOK();
        }
    }

    void TestSpeedSmall() {
        TestSpeed_(1000000);
    }
    void TestSpeed() {
        TestSpeed_(20000000);
    }
    void TestMemory() {
        typedef TSimpleAhoCorasickBuilder TAhoBuilder;
        typedef TSimpleMappedAhoCorasick TAhoSearcher;

        TTempFileHandle file("pure.bin");
        {
            TTimeLogger logger("Building the automaton from pure");
            TAhoBuilder ahoBuilder;
            TFileInput pure("data/pure.unpacked");

            TString line;
            size_t wordsCount = 0;
            while (pure.ReadLine(line)) {
                const static TSplitDelimiters DELIMS("\t");
                const TDelimitersSplit split(line, DELIMS);
                TDelimitersSplit::TIterator it = split.Iterator();

                const TString word = it.NextString();
                const ui32 freq = FromString<ui32>(it.NextString());
                ahoBuilder.AddString(word, freq);
                ++wordsCount;
            }
            Cout << "words added: " << wordsCount << ", trie vertices amount: " << ahoBuilder.AhoVertexes.size() << Endl;
            logger.SetOK();

            TTimeLogger logger2("Saving the result into blob");
            TBlob blob = ahoBuilder.Save();
            Cout << "data size: " << blob.Size() << Endl;
            TFixedBufferFileOutput output(file);
            output.Write(blob.Data(), blob.Size());
            logger2.SetOK();
        }
        {
            TTimeLogger logger("Searching queries");
            TBlob blob = TBlob::FromFile(file);
            TAhoSearcher ahoSearcher(blob);
            ahoSearcher.CheckData();
            TFileInput input("data/queries.txt");
            TString line;
            size_t linesCount = 0;
            size_t occurencesCount = 0;
            size_t searchedCharsCount = 0;
            while (input.ReadLine(line)) {
                ++linesCount;
                searchedCharsCount += line.size();
                occurencesCount += (ahoSearcher.AhoSearch(line)).size();
            }
            Cout << "lines count: " << linesCount << ", occurences count: " << occurencesCount
                 << ", searched chars count: " << searchedCharsCount << Endl;
            logger.SetOK();
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TTestMappedAhoCorasick);
