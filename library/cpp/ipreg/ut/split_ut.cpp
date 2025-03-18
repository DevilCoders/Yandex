#include <library/cpp/testing/unittest/env.h> // ArcadiaSourceRoot()
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/vector.h>

#include <library/cpp/ipreg/split.h>

using namespace NIPREG;

struct TSplitResult {
    TString First;
    TString Last;
    TString Data;
};

Y_UNIT_TEST_SUITE(SplitTest) {
    Y_UNIT_TEST(Split) {
        TReader reader(ArcadiaSourceRoot() + "/library/cpp/ipreg/ut/testIPREG.split", false, " ");

        TVector<TSplitResult> expected_all = {
            { "0000:0000:0000:0000:0000:0000:0000:0001", "0000:0000:0000:0000:0000:0000:0000:0001", "abcd" },
            { "0000:0000:0000:0000:0000:0000:0000:0002", "0000:0000:0000:0000:0000:0000:0000:0002", "cde" },
            { "0000:0000:0000:0000:0000:0000:0000:0003", "0000:0000:0000:0000:0000:0000:0000:0003", "df" },
            { "0000:0000:0000:0000:0000:0000:0000:0004", "0000:0000:0000:0000:0000:0000:0000:0004", "dfg" },
            { "0000:0000:0000:0000:0000:0000:0000:0005", "0000:0000:0000:0000:0000:0000:0000:0005", "d" },
            { "0000:0000:0000:0000:0000:0000:0000:0006", "0000:0000:0000:0000:0000:0000:0000:0006", "h" },
            { "0000:0000:0000:0000:0000:0000:0000:0008", "0000:0000:0000:0000:0000:0000:0000:0008", "i" },
        };

        auto expected = expected_all.begin();

        SplitIPREG(reader,
            [&](const TAddress& first, const TAddress& last, const TVector<TString>& data) {
                if (expected == expected_all.end()) {
                    UNIT_ASSERT(false);
                } else {
                    UNIT_ASSERT_STRINGS_EQUAL(first.AsIPv6(), expected->First);
                    UNIT_ASSERT_STRINGS_EQUAL(last.AsIPv6(), expected->Last);
                    UNIT_ASSERT_STRINGS_EQUAL(JoinStrings(data.begin(), data.end(), ""), expected->Data);
                    expected++;
                }
            }
        );

        UNIT_ASSERT_EQUAL(expected, expected_all.end());
    }

    Y_UNIT_TEST(SplitOnlyFinal) {
        TReverseByLastIpReader reader(ArcadiaSourceRoot() + "/library/cpp/ipreg/ut/testIPREG.split", false, " ");

        TVector<TSplitResult> expected_all = {
            { "0000:0000:0000:0000:0000:0000:0000:0001", "0000:0000:0000:0000:0000:0000:0000:0001", "a" },
            { "0000:0000:0000:0000:0000:0000:0000:0002", "0000:0000:0000:0000:0000:0000:0000:0002", "e" },
            { "0000:0000:0000:0000:0000:0000:0000:0003", "0000:0000:0000:0000:0000:0000:0000:0003", "f" },
            { "0000:0000:0000:0000:0000:0000:0000:0004", "0000:0000:0000:0000:0000:0000:0000:0004", "g" },
            { "0000:0000:0000:0000:0000:0000:0000:0005", "0000:0000:0000:0000:0000:0000:0000:0005", "d" },
            { "0000:0000:0000:0000:0000:0000:0000:0006", "0000:0000:0000:0000:0000:0000:0000:0006", "h" },
            { "0000:0000:0000:0000:0000:0000:0000:0008", "0000:0000:0000:0000:0000:0000:0000:0008", "i" },
        };

        auto expected = expected_all.begin();

        SplitIPREG(reader,
                   [&](const TAddress& first, const TAddress& last, const TVector<TString>& data) {
                       if (expected == expected_all.end()) {
                           UNIT_ASSERT(false);
                       } else {
                           UNIT_ASSERT_STRINGS_EQUAL(first.AsIPv6(), expected->First);
                           UNIT_ASSERT_STRINGS_EQUAL(last.AsIPv6(), expected->Last);
                           UNIT_ASSERT_STRINGS_EQUAL(data.back(), expected->Data);
                           expected++;
                       }
                   }
        );

        UNIT_ASSERT_EQUAL(expected, expected_all.end());
    }
}

Y_UNIT_TEST_SUITE(ReverseByLastIpReaderTest) {
    Y_UNIT_TEST(ReverseByLastIpReaderTest) {
        TReverseByLastIpReader reader(ArcadiaSourceRoot() + "/library/cpp/ipreg/ut/testIPREG.split", false, " ");

        TVector<TSplitResult> expected_all = {
            { "0000:0000:0000:0000:0000:0000:0000:0001", "0000:0000:0000:0000:0000:0000:0000:0005", "d" },
            { "0000:0000:0000:0000:0000:0000:0000:0001", "0000:0000:0000:0000:0000:0000:0000:0002", "c" },
            { "0000:0000:0000:0000:0000:0000:0000:0001", "0000:0000:0000:0000:0000:0000:0000:0001", "b" },
            { "0000:0000:0000:0000:0000:0000:0000:0001", "0000:0000:0000:0000:0000:0000:0000:0001", "a" },
            { "0000:0000:0000:0000:0000:0000:0000:0002", "0000:0000:0000:0000:0000:0000:0000:0002", "e" },
            { "0000:0000:0000:0000:0000:0000:0000:0003", "0000:0000:0000:0000:0000:0000:0000:0004", "f" },
            { "0000:0000:0000:0000:0000:0000:0000:0004", "0000:0000:0000:0000:0000:0000:0000:0004", "g" },
            { "0000:0000:0000:0000:0000:0000:0000:0006", "0000:0000:0000:0000:0000:0000:0000:0006", "h" },
            { "0000:0000:0000:0000:0000:0000:0000:0008", "0000:0000:0000:0000:0000:0000:0000:0008", "i" },
        };

        auto expected = expected_all.begin();

        while (reader.Next()) {
            UNIT_ASSERT_C(expected != expected_all.end(), "got more than expected");

            auto item = reader.Get();
            UNIT_ASSERT_STRINGS_EQUAL(item.First.AsIPv6(), expected->First);
            UNIT_ASSERT_STRINGS_EQUAL(item.Last.AsIPv6(), expected->Last);
            UNIT_ASSERT_STRINGS_EQUAL(item.Data, expected->Data);
            ++expected;
        }
        UNIT_ASSERT_C(expected == expected_all.end(), "got less than expected");
    }
}
