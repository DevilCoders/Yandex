#include "edit_distance.h"
#include "align_wtrokas.h"
#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/charset/ci_string.h>
#include <util/string/join.h>

#include <cstdio>

const double EPSILON = 1e-14;
const TString ModelFilename = "model.txt";
const TString BinModelFilename = "bin_model.txt";

void GenerateViterbiTable() {
    TUnbufferedFileOutput stream(ModelFilename);
    stream << "1\t1\t10\n";
    stream << "a\tb\t5\n";
    stream << "a\ta\t0\n";
    stream << "b\tb\t0\n";
    stream << "a\tE\t1\n";
    stream << "E\ta\t1\n";
}

void GenerateStochasticTable() {
    TUnbufferedFileOutput stream(ModelFilename);
    stream << "1\t1\t1e-16\n";
    stream << "a\tb\t0.01\n";
    stream << "b\ta\t0.015\n";

    stream << "a\ta\t0.02\n";
    stream << "b\tb\t0.02\n";
    stream << "c\tc\t0.001\n";

    stream << "c\ta\t0.1\n";
    stream << "a\tc\t0.1\n";

    stream << "c\tE\t0.01\n";
    stream << "E\tc\t0.01\n";

    stream << "a\tE\t0.01\n";
    stream << "#\t#\t0.5\n";
}

void GenerateContextTable() {
    TUnbufferedFileOutput stream(ModelFilename);
    stream << "2\t2\t2\t1\n";

    stream << "a\tb\ta\tb\tc\t0.5\n";
    stream << "^\ta\t^\ta\tc\t0.2\n";

    stream << "bb\tcc\tbb\tc\td\t0.01\n";
    stream << "b\tcc\tbb\tc\td\t0.1\n";

    stream << "x\tx\tx\tx\tE\t0.2\n";
    stream << "x\tE\tx\tx\tE\t0.02\n";

    stream << "y\tE\tE\ta\tz\t0.2\n";
}

template <typename T>
void TestDistance(const T& measurer, const TString& TCiString, double trueValue, bool calcLog) {
    TVector<TString> strs = SplitString(TCiString, " ");
    double value = measurer.CalcDistance(::UTF8ToWide(strs[0]), ::UTF8ToWide(strs[1]));

    UNIT_ASSERT(std::abs(value - (calcLog ? -log(trueValue) : trueValue)) < EPSILON);
}

void TestAlign(const TString& pair, int trueDiff) {
    TVector<TString> strs = SplitString(pair, "|");
    size_t diff = NEditDistance::CalculateDifference(NEditDistance::AlignWtrokas(::UTF8ToWide(strs[0]), ::UTF8ToWide(strs[1])));
    UNIT_ASSERT_VALUES_EQUAL(trueDiff, diff);
}

Y_UNIT_TEST_SUITE(TEditDistanceTest) {
    Y_UNIT_TEST(TViterbiTest) {
        GenerateViterbiTable();
        NEditDistance::TViterbiLevenshtein viterbi;

        viterbi.ReadModel(ModelFilename);
        viterbi.SaveModel(BinModelFilename);

        viterbi = NEditDistance::TViterbiLevenshtein();
        viterbi.LoadModel(BinModelFilename);
        TestDistance(viterbi, "a b", 5, false);
        TestDistance(viterbi, "aaa aaa", 0, false);
        TestDistance(viterbi, "aa aaa", 1, false);
        TestDistance(viterbi, "aaa aa", 1, false);
        TestDistance(viterbi, "abba baab", 4, false);
        TestDistance(viterbi, "aaa aba", 5, false);
        TestDistance(viterbi, "aa aba", 6, false);
        std::remove(BinModelFilename.c_str());
        std::remove(ModelFilename.c_str());
    }

    Y_UNIT_TEST(TStochasticTest) {
        GenerateStochasticTable();
        NEditDistance::TStochasticLevenshtein stochastic;

        stochastic.ReadModel(ModelFilename);
        stochastic.SaveModel(BinModelFilename);

        stochastic = NEditDistance::TStochasticLevenshtein();
        stochastic.LoadModel(BinModelFilename);

        TestDistance(stochastic, "a b", 0.01 * 0.5, true);
        TestDistance(stochastic, "a a", 0.02 * 0.5, true);

        // consists of 3 pathes: (c, E)(E, c) = 0.01 * 0.01; (E, c)(c, E) = 0.001; (c, c) = 0.01 * 0.01
        TestDistance(stochastic, "c c", 0.0012 * 0.5, true);
        TestDistance(stochastic, "aa aa", 0.02 * 0.02 * 0.5, true);
        TestDistance(stochastic, "ba ab", 0.01 * 0.015 * 0.5, true);
        TestDistance(stochastic, "b d", 1e-16 * 0.5, true);
        std::remove(BinModelFilename.c_str());
        std::remove(ModelFilename.c_str());
    }

    Y_UNIT_TEST(TContextTest) {
        GenerateContextTable();
        NEditDistance::TContextLevenshtein context;

        context.ReadModel(ModelFilename);
        context.SaveModel(BinModelFilename);

        context = NEditDistance::TContextLevenshtein();
        context.LoadModel(BinModelFilename);
        TestDistance(context, "ab ac", 0.5 / (2 + 2), false);
        TestDistance(context, "aba aba", 0, false);
        TestDistance(context, "a c", 0.2 / (1 + 1), false);
        TestDistance(context, "bbcc bbdc", 0.01 / (4 + 4), false);
        TestDistance(context, "xx x", 0.2 / (2 + 1), false);
        TestDistance(context, "ya yz", 0.2 / (2 + 2), false);
        std::remove(BinModelFilename.c_str());
        std::remove(ModelFilename.c_str());
    }

    Y_UNIT_TEST(TAlignmentTest) {
        TestAlign("xx xxx xx|xx xxx xx", 0);
        TestAlign("xx xx xxx|xx xxx xxx", 1);
        TestAlign("x x xxx xxx|xx xxx xxx", 1);
        TestAlign("x x x x xxx|xxxx xxx", 3);
        TestAlign("xx xx xx xx|xxxx xxxx", 2);
        TestAlign("xxx x xx|xxx xxx", 1);
        TestAlign("xxx  x  xx|xxx xxx", 1);
    }
}
