#include "unique_results.h"
#include "find_solutions.h"
#include "solve_ambiguity.h"
#include "occurrence.h"
#include "solution.h"
#include "order.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/str.h>
#include <util/generic/strbuf.h>

using namespace NSolveAmbig;

TString ToString(TVector<TOccurrence>& res) {
    if (res.empty()) {
        return TString();
    }
    ::StableSort(res.begin(), res.end(), NImpl::TOccurrenceInfoOrder());
    TStringStream str;
    str << res.front().Info;
    for (size_t i = 1; i < res.size(); ++i) {
        str << ',' << res[i].Info;
    }
    return str.Str();
}

TString ToString(const TVector<TOccurrence>& res, const TVector<size_t>& off) {
    if (off.empty()) {
        return TString();
    }
    TStringStream str;
    str << res[off.front()].Info;
    for (size_t i = 1; i < off.size(); ++i) {
        str << ',' << res[off[i]].Info;
    }
    return str.Str();
}

TOccurrence MakeOcc(size_t beg, size_t end, size_t info, double weight = 0.0) {
    TOccurrence occ(std::make_pair(beg, end));
    occ.Info = info;
    occ.Weight = weight;
    return occ;
}

Y_UNIT_TEST_SUITE(UniqueResults) {
    Y_UNIT_TEST(Empty) {
        TVector<TOccurrence> res;
        MakeUniqueResults(res);
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf());
    }

    Y_UNIT_TEST(Single) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(0, 1, 1));
        MakeUniqueResults(res);
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("1"));
    }

    Y_UNIT_TEST(MakeUnique) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(0, 5, 1));
        res.push_back(MakeOcc(0, 3, 2));
        res.push_back(MakeOcc(3, 4, 3));
        res.push_back(MakeOcc(0, 3, 4)); // duplicate of 1
        res.push_back(MakeOcc(0, 5, 5)); // duplicate of 0

        MakeUniqueResults(res);
        ::StableSort(res.begin(), res.end(), NImpl::TOccurrenceInfoOrder());
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("1,2,3"));
    }

    Y_UNIT_TEST(MakeUniqueWeighted) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(0, 5, 1, 0.3));
        res.push_back(MakeOcc(0, 3, 2, 0.5));
        res.push_back(MakeOcc(3, 4, 3));
        res.push_back(MakeOcc(0, 3, 4, 0.2)); // duplicate of 1
        res.push_back(MakeOcc(0, 5, 5, 1.3)); // duplicate of 0

        MakeUniqueResults(res);
        ::StableSort(res.begin(), res.end(), NImpl::TOccurrenceInfoOrder());
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("2,3,5"));
    }
}

#define CHECK_SOLUTION(items, sol, expected) \
    UNIT_ASSERT_EQUAL_C(ToString(items, (sol).Positions), expected, ", actual: " << ToString(items, (sol).Positions));

Y_UNIT_TEST_SUITE(FindSolutions) {
    Y_UNIT_TEST(Solutions1) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(4, 9, 1));
        res.push_back(MakeOcc(0, 4, 3));
        res.push_back(MakeOcc(1, 5, 2));
        res.push_back(MakeOcc(5, 9, 4));

        TVector<TSolutionPtr> solutions;
        FindAllSolutions(res, solutions);
        UNIT_ASSERT_EQUAL(solutions.size(), 3);
        CHECK_SOLUTION(res, *solutions[0], "3,1");
        CHECK_SOLUTION(res, *solutions[1], "2,4");
        CHECK_SOLUTION(res, *solutions[2], "3,4");
    }
    Y_UNIT_TEST(Solutions2) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(0, 4, 1));
        res.push_back(MakeOcc(4, 9, 2));
        res.push_back(MakeOcc(0, 9, 3));

        TVector<TSolutionPtr> solutions;
        FindAllSolutions(res, solutions);
        UNIT_ASSERT_EQUAL(solutions.size(), 2);
        CHECK_SOLUTION(res, *solutions[0], "3");
        CHECK_SOLUTION(res, *solutions[1], "1,2");
    }
    Y_UNIT_TEST(SplitPath) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(0, 4, 1));
        res.push_back(MakeOcc(4, 9, 2));
        res.push_back(MakeOcc(4, 7, 3));

        TVector<TSolutionPtr> solutions;
        FindAllSolutions(res, solutions);
        UNIT_ASSERT_EQUAL(solutions.size(), 2);
        CHECK_SOLUTION(res, *solutions[0], "1,2");
        CHECK_SOLUTION(res, *solutions[1], "1,3");
    }
    Y_UNIT_TEST(MergePath) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(0, 4, 1));
        res.push_back(MakeOcc(2, 4, 2));
        res.push_back(MakeOcc(4, 9, 3));

        TVector<TSolutionPtr> solutions;
        FindAllSolutions(res, solutions);
        UNIT_ASSERT_EQUAL(solutions.size(), 2);
        CHECK_SOLUTION(res, *solutions[0], "1,3");
        CHECK_SOLUTION(res, *solutions[1], "2,3");
    }
    Y_UNIT_TEST(MultiPath) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(0, 4, 1));
        res.push_back(MakeOcc(2, 4, 2));
        res.push_back(MakeOcc(4, 9, 3));
        res.push_back(MakeOcc(4, 7, 4));

        TVector<TSolutionPtr> solutions;
        FindAllSolutions(res, solutions);
        UNIT_ASSERT_EQUAL(solutions.size(), 4);
        CHECK_SOLUTION(res, *solutions[0], "1,3");
        CHECK_SOLUTION(res, *solutions[1], "2,3");
        CHECK_SOLUTION(res, *solutions[2], "1,4");
        CHECK_SOLUTION(res, *solutions[3], "2,4");
    }
}

Y_UNIT_TEST_SUITE(SolveAmbig) {
    Y_UNIT_TEST(DefaultRank) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(4, 9, 1));
        res.push_back(MakeOcc(0, 4, 2));
        res.push_back(MakeOcc(1, 9, 3));
        SolveAmbiguity(res);
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("1,2"));

        res.clear();
        res.push_back(MakeOcc(4, 9, 1));
        res.push_back(MakeOcc(0, 4, 2));
        res.push_back(MakeOcc(0, 9, 3));
        SolveAmbiguity(res);
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("3"));

        res.clear();
        res.push_back(MakeOcc(0, 9, 1));
        res.push_back(MakeOcc(0, 9, 2));
        SolveAmbiguity(res);
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("1"));

        res.clear();
        res.push_back(MakeOcc(0, 9, 1));
        res.push_back(MakeOcc(0, 9, 2, 0.4));
        SolveAmbiguity(res);
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("2"));
    }

    Y_UNIT_TEST(WeightRank) {
        TVector<TOccurrence> res;
        res.push_back(MakeOcc(4, 9, 1));
        res.push_back(MakeOcc(0, 4, 2));
        res.push_back(MakeOcc(1, 9, 3, 1.3));
        SolveAmbiguity(res, TRankMethod(1, RC_G_WEIGHT));
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("3"));

        res.clear();
        res.push_back(MakeOcc(0, 9, 1));
        res.push_back(MakeOcc(0, 9, 2));
        SolveAmbiguity(res);
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("1"));

        res.clear();
        res.push_back(MakeOcc(0, 9, 1));
        res.push_back(MakeOcc(0, 9, 2, 0.4));
        SolveAmbiguity(res);
        UNIT_ASSERT_EQUAL(ToString(res), TStringBuf("2"));
    }
}
