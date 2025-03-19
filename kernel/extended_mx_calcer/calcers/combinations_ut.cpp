#include "combinations.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/ut_utils/scheme_ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NExtendedMx;
using namespace NSc::NUt;


class TCombinatorTest : public NUnitTest::TTestBase {
    UNIT_TEST_SUITE(TCombinatorTest);
        UNIT_TEST(TestGoodLoad);
        UNIT_TEST(TestGoodLoadWithNoPosition);
        UNIT_TEST(TestBadLoadWithIncorrectFeatIndex);
        UNIT_TEST(TestBadLoadWithMissedFeats);
        UNIT_TEST(TestBadLoadWithDuplicateCombinations);
        UNIT_TEST(TestInnerCombinationsSorted);
        UNIT_TEST(TestIntersectWithAvailVTCombGood);
        UNIT_TEST(TestIntersectBinaryWithAvailVTCombWithNoShowGood);
        UNIT_TEST(TestIntersectBinaryWithAvailVTCombGood);
        UNIT_TEST(TestFillAll);
        UNIT_TEST(TestCombinationInfo);
    UNIT_TEST_SUITE_END();

public:
    void TestGoodLoad() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 0, 0 ], [ 1, 0 ], [ 2, 1 ] ],
                "Features": [
                    { "Name": "Pos", "Values": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ] },
                    { "Name": "ViewType", "Values": [ "xl", "m" ] }
                ],
                "NoShow": [9, 0]
            }
        )");
        UNIT_ASSERT_NO_EXCEPTION(NCombinator::TCombinator(scheme));
    }

    void TestGoodLoadWithNoPosition() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 0, 0 ], [ 1, 0 ], [ 2, 1 ], [ 3, 0 ] ],
                "Features": [
                    { "Name": "Pos", "Values": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ] },
                    { "Name": "ViewType", "Values": [ "xl", "m" ] }
                ],
                "NoShow": [0, 1]
            }
        )");
        UNIT_ASSERT_NO_EXCEPTION(NCombinator::TCombinator(scheme));
    }

    void TestBadLoadWithIncorrectFeatIndex() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 0, 0 ], [ 1, 0 ], [ 2, 1 ], [ 3, 0 ] ],
                "Features": [
                    { "Name": "Pos", "Values": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ] },
                    { "Name": "ViewType", "Values": [ "xl", "m" ] }
                ],
                "NoShow": [3, 3]
            }
        )");
        UNIT_ASSERT_EXCEPTION(NCombinator::TCombinator(scheme), yexception);
    }

    void TestBadLoadWithMissedFeats() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 0, 0 ], [ 1, 0 ], [ 2, 1 ], [ 3, 0 ] ],
                "Features": [
                    { "Name": "Pos", "Values": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ] },
                ],
                "NoShow": [0, 1]
            }
        )");
        UNIT_ASSERT_EXCEPTION(NCombinator::TCombinator(scheme), yexception);
    }

    void TestBadLoadWithDuplicateCombinations() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 0, 0 ], [ 1, 0 ], [ 2, 1 ], [ 3, 0 ] ],
                "Features": [
                    { "Name": "Pos", "Values": [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ] },
                    { "Name": "ViewType", "Values": [ "xl", "m" ] }
                ],
                "NoShow": [0, 0],
                "UseNoShowAsCombination": 1
            }
        )");
        UNIT_ASSERT_EXCEPTION(NCombinator::TCombinator(scheme), yexception);
    }

    void TestInnerCombinationsSorted() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 2, 1 ], [2, 0], [ 1, 0 ], [ 0, 0 ] ],
                "Features": [
                    { "Name": "Pos", "Values": [ 0, 1, 2, 3, 4] },
                    { "Name": "ViewType", "Values": [ "xl", "m" ] }
                ],
                "NoShow": [4, 0]
            }
        )");
        NSc::TValue inner = NCombinator::TCombinator(scheme).ToJsonDebug();
        AssertSchemeJson(R"([[2,1],[2,0],[1,0],[0,0]])", inner["AllCombinations"]);
        AssertSchemeJson(R"([["2","m"],["2","xl"],["1","xl"],["0","xl"]])", inner["AllCombinationValues"]);
        AssertSchemeJson(R"({"Pos":["0","1","2","3","4"],"ViewType":["xl","m"]})", inner["Feature2Idx2Value"]);
        AssertSchemeJson(R"({"Pos":{"0":0,"1":1,"2":2,"3":3,"4":4},"ViewType":{"m":1,"xl":0}})", inner["Feature2Value2Idx"]);
        AssertSchemeJson(R"([{"IsBinary":0,"Name":"Pos"},{"IsBinary":0,"Name":"ViewType"}])", inner["FeaturesOrder"]);
    }

    void TestIntersectWithAvailVTCombGood() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 1, 0, 2 ], [0, 1, 2], [ 0, 0, 1 ], [ 0, 0, 0 ] ],
                "Features": [
                    { "Name": "ViewType", "Values": [ "xl", "m" ] },
                    { "Name": "Placement", "Values": [ "Main", "Right" ] },
                    { "Name": "Pos", "Values": [ 0, 1, 2, 3, 4] }
                ],
                "NoShow": [0, 0, 4]
            }
        )");
        auto availVTComb = NSc::TValue::FromJsonThrow(R"({"m":{"Main":"*"},"xl":{"Main":[0]} })");

        NCombinator::TCombinator combinator(scheme);
        auto idxs = combinator.Intersect(TAvailVTCombinationsConst(&availVTComb));
        AssertSchemeJson("[0, 3]", NJsonConverters::ToTValue(idxs));

        TVector<float> feats = {-1};
        auto resFeats = combinator.FillWithFeatures(idxs, feats.data(), feats.size());
        AssertSchemeJson("[[1, 0, 2, -1], [0, 0, 0, -1]]", NJsonConverters::ToTValue(resFeats));
    }

    void TestIntersectBinaryWithAvailVTCombGood() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 1, 0, 2 ], [0, 1, 2], [ 0, 0, 1 ], [ 0, 0, 0 ] ],
                "Features": [
                    { "Binary": 1, "Name": "ViewType", "Values": [ "xl", "m" ] },
                    { "Binary": 1, "Name": "Placement", "Values": [ "Main", "Right" ] },
                    { "Binary": 0, "Name": "Pos", "Values": [ 0, 1, 2, 3, 4] }
                ],
                "NoShow": [0, 0, 4]
            }
        )");
        auto availVTComb = NSc::TValue::FromJsonThrow(R"({"m":{"Main":"*"},"xl":{"Main":[0]}})");

        NCombinator::TCombinator combinator(scheme);
        auto idxs = combinator.Intersect(TAvailVTCombinationsConst(&availVTComb));
        AssertSchemeJson("[0, 3]", NJsonConverters::ToTValue(idxs));

        TVector<float> feats = {-1};
        auto resFeats = combinator.FillWithFeatures(idxs, feats.data(), feats.size());
        AssertSchemeJson("[[0, 1, 1, 0, 2, -1], [1, 0, 1, 0, 0, -1]]", NJsonConverters::ToTValue(resFeats));
    }

    void TestIntersectBinaryWithAvailVTCombWithNoShowGood() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 1, 0, 2 ], [0, 1, 2 ], [ 0, 0, 1 ], [ 0, 0, 0 ] ],
                "Features": [
                    { "Binary": 1, "Name": "ViewType", "Values": [ "xl", "m" ] },
                    { "Binary": 1, "Name": "Placement", "Values": [ "Main", "Right" ] },
                    { "Binary": 0, "Name": "Pos", "Values": [ 0, 1, 2, 3, 4] }
                ],
                "NoShow": [0, 0, 4],
                "UseNoShowAsCombination": 1
            }
        )");
        auto availVTComb = NSc::TValue::FromJsonThrow(R"({"m":{"Main":"*"},"xl":{"Main":[0]}})");

        NCombinator::TCombinator combinator(scheme);
        auto idxs = combinator.Intersect(TAvailVTCombinationsConst(&availVTComb));
        AssertSchemeJson("[0, 3, 4]", NJsonConverters::ToTValue(idxs));

        TVector<float> feats = {-1};
        auto resFeats = combinator.FillWithFeatures(idxs, feats.data(), feats.size());
        AssertSchemeJson("[[0, 1, 1, 0, 2, -1], [1, 0, 1, 0, 0, -1], [1, 0, 1, 0, 4, -1]]", NJsonConverters::ToTValue(resFeats));
    }

    void TestFillAll() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 1, 0, 2 ], [0, 1, 2], [ 0, 0, 1 ], [ 0, 0, 0 ] ],
                "Features": [
                    { "Binary": 1, "Name": "ViewType", "Values": [ "xl", "m" ] },
                    { "Binary": 1, "Name": "Placement", "Values": [ "Main", "Right" ] },
                    { "Binary": 0, "Name": "Pos", "Values": [ 0, 1, 2, 3, 4] }
                ],
                "NoShow": [0, 0, 4],
                "UseNoShowAsCombination": 1
            }
        )");
        NCombinator::TCombinator combinator(scheme);
        TVector<float> feats = {-1};
        auto resFeats = combinator.FillWithFeatures(feats.data(), feats.size());
        AssertSchemeJson("[[0,1,1,0,2,-1],[1,0,0,1,2,-1],[1,0,1,0,1,-1],[1,0,1,0,0,-1],[1,0,1,0,4,-1]]", NJsonConverters::ToTValue(resFeats));
    }

    void TestCombinationInfo() {
        auto scheme = NSc::TValue::FromJsonThrow(R"( {
                "AllowedCombinations": [ [ 2, 1, 0 ], [2, 0, 1], [ 1, 0, 0 ], [ 0, 0, 0 ] ],
                "Features": [
                    { "Binary": 0, "Name": "Pos", "Values": [ 0, 1, 2, 3, 4] },
                    { "Binary": 1, "Name": "ViewType", "Values": [ "xl", "m" ] },
                    { "Binary": 1, "Name": "Placement", "Values": [ "Main", "Right" ] }
                ],
                "NoShow": [4, 0, 0]
            }
        )");
        NCombinator::TCombinator combinator(scheme);
        AssertSchemeJson(R"({"Placement":"Main","Pos":2,"ViewType":"m"})", combinator.GetCombinationValues(0));
        AssertSchemeJson(R"({"Placement":"Right","Pos":2,"ViewType":"xl"})", combinator.GetCombinationValues(1));
        AssertSchemeJson(R"({"Placement":"Main","Pos":1,"ViewType":"xl"})", combinator.GetCombinationValues(2));
        AssertSchemeJson(R"({"Placement":"Main","Pos":0,"ViewType":"xl"})", combinator.GetCombinationValues(3));
        UNIT_ASSERT_EXCEPTION(combinator.GetCombinationValues(4), yexception);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCombinatorTest);
