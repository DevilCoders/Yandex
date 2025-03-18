#include <library/cpp/testing/unittest/env.h> // ArcadiaSourceRoot()
#include <library/cpp/testing/unittest/registar.h>

#include <util/string/vector.h>

#include <library/cpp/ipreg/merge.h>

using namespace NIPREG;

struct TMergeResult {
    TString First;
    TString Last;
    TString DataA;
    TString DataB;
};

Y_UNIT_TEST_SUITE(MergeTest) {
    Y_UNIT_TEST(Merge) {
        TReader readera(ArcadiaSourceRoot() + "/library/cpp/ipreg/ut/testIPREG.merge.a", false, " ");
        TReader readerb(ArcadiaSourceRoot() + "/library/cpp/ipreg/ut/testIPREG.merge.b", false, " ");

        TVector<TMergeResult> expected_all = {
            { "0000:0000:0000:0000:0000:0000:0000:0000", "0000:0000:0000:0000:0000:0000:0000:0000", "-", "-" },
            { "0000:0000:0000:0000:0000:0000:0000:0001", "0000:0000:0000:0000:0000:0000:0000:0001", "a1", "b1" },
            { "0000:0000:0000:0000:0000:0000:0000:0002", "0000:0000:0000:0000:0000:0000:0000:0002", "a1", "-" },
            { "0000:0000:0000:0000:0000:0000:0000:0003", "0000:0000:0000:0000:0000:0000:0000:0003", "a1", "b2" },
            { "0000:0000:0000:0000:0000:0000:0000:0004", "0000:0000:0000:0000:0000:0000:0000:0004", "a1", "-" },
            { "0000:0000:0000:0000:0000:0000:0000:0005", "0000:0000:0000:0000:0000:0000:0000:0005", "a1", "b3" },
            { "0000:0000:0000:0000:0000:0000:0000:0006", "0000:0000:0000:0000:0000:0000:0000:0006", "a2", "b3" },
            { "0000:0000:0000:0000:0000:0000:0000:0007", "0000:0000:0000:0000:0000:0000:0000:0007", "-", "-" },
            { "0000:0000:0000:0000:0000:0000:0000:0008", "0000:0000:0000:0000:0000:0000:0000:0008", "a3", "b4" },
            { "0000:0000:0000:0000:0000:0000:0000:0009", "0000:0000:0000:0000:0000:0000:0000:0010", "-", "b4" },
            { "0000:0000:0000:0000:0000:0000:0000:0011", "0000:0000:0000:0000:0000:0000:0000:0011", "-", "-" },
            { "0000:0000:0000:0000:0000:0000:0000:0012", "0000:0000:0000:0000:0000:0000:0000:0012", "a4", "-" },
            { "0000:0000:0000:0000:0000:0000:0000:0013", "0000:0000:0000:0000:0000:0000:0000:0014", "a4", "b5" },
            { "0000:0000:0000:0000:0000:0000:0000:0015", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "-", "-" },
        };

        auto expected = expected_all.begin();

        MergeIPREGS(readera, readerb,
            [&](const TAddress& first, const TAddress& last, const TString *a, const TString *b) {
                if (expected == expected_all.end()) {
                    UNIT_ASSERT(false);
                } else {
                    UNIT_ASSERT_STRINGS_EQUAL(first.AsIPv6(), expected->First);
                    UNIT_ASSERT_STRINGS_EQUAL(last.AsIPv6(), expected->Last);
                    UNIT_ASSERT_STRINGS_EQUAL(a ? *a : "-", expected->DataA);
                    UNIT_ASSERT_STRINGS_EQUAL(b ? *b : "-", expected->DataB);
                    expected++;
                }
            }
        );

        UNIT_ASSERT_EQUAL(expected, expected_all.end());
    }
}
