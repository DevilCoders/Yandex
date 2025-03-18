#include "map_text_file.h"
#include "map_tsv_file.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>
#include <util/generic/vector.h>

class FileSplitterTest: public TTestBase {
    UNIT_TEST_SUITE(FileSplitterTest);
    UNIT_TEST(TestFieldsSplit);
    UNIT_TEST(TestCRLF);
    UNIT_TEST(TestEOL);
    UNIT_TEST(TestMapTextFile);
    UNIT_TEST_SUITE_END();

public:
    void AssertEqualFields(const std::vector<std::vector<TString>>& expectedFieldsList, const TString& filename, const TString& input) {
        TUnbufferedFileOutput f(filename);
        f.Write(TStringBuf(input));

        if (false) {
            // UNIT_ASSERT_EQUAL now have not very readable diagnostic for non-equal containers
            Cerr << "START" << Endl;
            Cerr << "=============================" << Endl;
            for (auto fs : expectedFieldsList) {
                for (auto f : fs) {
                    Cerr << f.Quote() << "; ";
                }
            }
            Cerr << Endl;

            Cerr << "------------------" << Endl;
            for (auto fs : TMapTsvFile(filename)) {
                for (auto f : fs) {
                    Cerr << TString(f).Quote() << "; ";
                }
            }
            Cerr << Endl;
            Cerr << "END" << Endl;
        }

        size_t iLine = 0;
        for (auto& fileFields : TMapTsvFile(filename)) {
            UNIT_ASSERT(iLine <= expectedFieldsList.size());
            auto& expectedFields = expectedFieldsList.at(iLine);
            UNIT_ASSERT_EQUAL(fileFields.size(), expectedFields.size());
            UNIT_ASSERT(std::equal(fileFields.begin(), fileFields.end(), expectedFields.begin()));
            ++iLine;
        };
        UNIT_ASSERT_EQUAL(iLine, expectedFieldsList.size());
        unlink(filename.c_str());
    }

    void TestFieldsSplit() {
        AssertEqualFields(
            {
                {"opqiuwhep", "iufhp"},
                {"qio", "euhlq"},
                {"", "wre"},
                {
                    "",
                    "",
                },
                {"quwehofi", ""},
            },
            "TestFieldsSplit.tmp",
            "opqiuwhep\tiufhp\nqio\teuhlq\n\twre\n\t\nquwehofi\t\n");
    }

    void TestEOL() {
        AssertEqualFields(
            {
                {"a"},
                {"b"},
            },
            "TestEOL_1.tmp",
            "a\nb");
        AssertEqualFields(
            {
                {"a"},
                {""},
                {"b"},
            },
            "TestEOL_1.tmp",
            "a\n\nb");
        AssertEqualFields(
            {
                {"a"},
                {"b"},
            },
            "TestEOL_2.tmp",
            "a\nb\n");
        AssertEqualFields(
            {
                {"a"},
            },
            "TestEOL_3.tmp",
            "a\n");
        AssertEqualFields(
            {
                {""},
            },
            "TestEOL_4.tmp",
            "\n");
        AssertEqualFields(
            {},
            "TestEOL_5.tmp",
            "");
    }

    template <typename F, typename TElem>
    void EnumerateCombinations(int numElements, const TElem* elements, TElem* result, size_t resultLen, size_t curPos, F f) {
        if (curPos == resultLen) {
            f(result, resultLen);
        } else {
            for (int i = 0; i < numElements; ++i) {
                result[curPos] = elements[i];
                EnumerateCombinations(numElements, elements, result, resultLen, curPos + 1, f);
            }
        }
    }

    static std::vector<TStringBuf> SplitLinesByEOL(char* result, size_t len) {
        // define in most straightforward and trivial way what we want from automatic line splitting.

        std::vector<std::pair<size_t, size_t>> eolPositions;
        for (size_t i = 0; i < len; ++i) {
            if (result[i] == '\n') {
                if (i > 0 && result[i - 1] == '\r') {
                    eolPositions.push_back({i - 1, i + 1});
                } else {
                    eolPositions.push_back({i, i + 1});
                }
            }
        }
        bool eolAtEndOfFile = !eolPositions.empty() && eolPositions.back().second == len;
        bool emptyFile = (len == 0);
        if (!emptyFile) {
            if (!eolAtEndOfFile) {
                eolPositions.push_back({len, len});
            }
        }
        for (size_t i = 0; (i + 1) < eolPositions.size(); ++i) {
            Y_ENSURE(eolPositions[i].second <= eolPositions[i + 1].first, "internal error, intersection of EOLS");
        }

        std::vector<TStringBuf> expectedSplit;
        size_t lineStart = 0;
        for (auto eol : eolPositions) {
            expectedSplit.push_back(TStringBuf(result + lineStart, result + eol.first));
            lineStart = eol.second;
        }
        return expectedSplit;
    }

    static void CheckCrLfOneCase(char* result, size_t len) {
        std::vector<TStringBuf> expectedSplit = SplitLinesByEOL(result, len);

        TIterTextLines iterLines(result, len);
        TIterTextLines end;

        std::vector<TStringBuf> resultSplit(iterLines, end);

        UNIT_ASSERT_EQUAL(expectedSplit, resultSplit);
    }

    void TestCRLF() {
        AssertEqualFields(
            {
                {"opqiuwhep"},
                {"wre\rzzz"},
                {"qqq"},
            },
            "TestCRLF.tmp",
            "opqiuwhep\r\nwre\rzzz\nqqq");

        AssertEqualFields({}, "TestCRLF_1.tmp", "");
        AssertEqualFields({{""}}, "TestCRLF_2.tmp", "\n");
        AssertEqualFields({{""}}, "TestCRLF_3.tmp", "\r\n");
        AssertEqualFields({{"\r"}}, "TestCRLF_4.tmp", "\r");
        AssertEqualFields({{"x"}}, "TestCRLF_5.tmp", "x");
        AssertEqualFields({{"x"}}, "TestCRLF_6.tmp", "x\n");
        AssertEqualFields({{"x\r"}}, "TestCRLF_7.tmp", "x\r");
        AssertEqualFields({{"x"}}, "TestCRLF_8.tmp", "x\r\n");

        // enumerate all combinations of '\r' '\n' and usual char
        const int N = 6;
        char result[N] = {};
        for (size_t len = 0; len <= N; ++len) {
            TStringBuf elements = "Z\r\n";
            EnumerateCombinations(elements.size(), elements.data(), result, len, 0, CheckCrLfOneCase);
        }
    }

    // TMapTextFile is already tested by it's wrapper TMapTsvFile, but let's have at least one trivial test
    void TestMapTextFile() {
        const TString filename = "TestMapTextFile.tmp";
        TUnbufferedFileOutput f(filename);
        f.Write("a\nb");

        TMapTextFile txtFile(filename);
        std::vector<TString> lines(txtFile.begin(), txtFile.end());
        std::vector<TString> expected = {"a", "b"};
        UNIT_ASSERT_EQUAL(txtFile.LineCount(), 2);
        UNIT_ASSERT_EQUAL(lines, expected);
        unlink(filename.c_str());

        // range-for interface
        size_t i = 0;
        for (auto l : txtFile) {
            UNIT_ASSERT_EQUAL(l, expected.at(i++));
        }
        UNIT_ASSERT_EQUAL(i, expected.size());
    }
};

UNIT_TEST_SUITE_REGISTRATION(FileSplitterTest);
