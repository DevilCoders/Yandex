#include <library/cpp/libgit2_wrapper/unidiff.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>

using namespace NLibgit2;

namespace {

TString Diff(
    const TStringBuf from,
    const TStringBuf to,
    const ui32 context = 3,
    bool colored = false,
    bool ignoreWhitespace = false,
    bool ignoreWhitespaceChange = false,
    bool ignoreWhitespaceEOL = false)
{
    TString diff;
    TStringOutput out(diff);
    UnifiedDiff(from,
                to,
                context,
                out,
                colored,
                ignoreWhitespace,
                ignoreWhitespaceChange,
                ignoreWhitespaceEOL);
    return diff;
}

} // namespace

Y_UNIT_TEST_SUITE(TUnidiffTests) {
    Y_UNIT_TEST(NoContext) {
        UNIT_ASSERT_EQUAL(
            Diff(
                "aaa\n",

                "bbb\n"
            ),
            "@@ -1 +1 @@\n"
            "-aaa\n"
            "+bbb\n"
        );
    }

    Y_UNIT_TEST(Context) {
        UNIT_ASSERT_EQUAL(
            Diff(
                "aaa\n"
                "bbb\n",

                "AAA\n"
                "bbb\n"
            ),
            "@@ -1,2 +1,2 @@\n"
            "-aaa\n"
            "+AAA\n"
            " bbb\n"
        );

        UNIT_ASSERT_EQUAL(
            Diff(
                "aaa\n"
                "bbb\n",

                "AAA\n"
                "bbb\n",

                5
            ),
            "@@ -1,2 +1,2 @@\n"
            "-aaa\n"
            "+AAA\n"
            " bbb\n"
        );

        UNIT_ASSERT_EQUAL(
            Diff(
                "aaa\n"
                "bbb\n",

                "aaa\n"
                "BBB\n"
            ),
            "@@ -1,2 +1,2 @@\n"
            " aaa\n"
            "-bbb\n"
            "+BBB\n"
        );

        UNIT_ASSERT_EQUAL(
            Diff(
                "aaa\n"
                "bbb\n"
                "ccc\n"
                "ddd\n"
                "eee\n"
                "fff\n"
                "ggg\n"
                "hhh\n"
                "iii\n",

                "aaa\n"
                "bbb\n"
                "ccc\n"
                "ddd\n"
                "EEE\n"
                "fff\n"
                "ggg\n"
                "hhh\n"
                "iii\n"
            ),
            "@@ -2,7 +2,7 @@ aaa\n"
            " bbb\n"
            " ccc\n"
            " ddd\n"
            "-eee\n"
            "+EEE\n"
            " fff\n"
            " ggg\n"
            " hhh\n"
        );
    }

    Y_UNIT_TEST(InterhunkContext) {
        UNIT_ASSERT_EQUAL(
            Diff(
                "aaa\n"
                "111\n"
                "222\n"
                "333\n"
                "444\n"
                "555\n"
                "666\n"
                "bbb\n",

                "AAA\n"
                "111\n"
                "222\n"
                "333\n"
                "444\n"
                "555\n"
                "666\n"
                "BBB\n"
            ),
            "@@ -1,8 +1,8 @@\n"
            "-aaa\n"
            "+AAA\n"
            " 111\n"
            " 222\n"
            " 333\n"
            " 444\n"
            " 555\n"
            " 666\n"
            "-bbb\n"
            "+BBB\n"
        );

        UNIT_ASSERT_EQUAL(
            Diff(
                "aaa\n"
                "111\n"
                "222\n"
                "333\n"
                "444\n"
                "555\n"
                "666\n"
                "777\n"
                "bbb\n",

                "AAA\n"
                "111\n"
                "222\n"
                "333\n"
                "444\n"
                "555\n"
                "666\n"
                "777\n"
                "BBB\n"
            ),
            "@@ -1,4 +1,4 @@\n"
            "-aaa\n"
            "+AAA\n"
            " 111\n"
            " 222\n"
            " 333\n"
            "@@ -6,4 +6,4 @@ aaa\n"
            " 555\n"
            " 666\n"
            " 777\n"
            "-bbb\n"
            "+BBB\n"
        );

        UNIT_ASSERT_EQUAL(
            Diff(
                "aaa\n"
                "111\n"
                "222\n"
                "333\n"
                "444\n"
                "555\n"
                "666\n"
                "777\n"
                "bbb\n",

                "AAA\n"
                "111\n"
                "222\n"
                "333\n"
                "444\n"
                "555\n"
                "666\n"
                "777\n"
                "BBB\n",

                2
            ),
            "@@ -1,3 +1,3 @@\n"
            "-aaa\n"
            "+AAA\n"
            " 111\n"
            " 222\n"
            "@@ -7,3 +7,3 @@ aaa\n"
            " 666\n"
            " 777\n"
            "-bbb\n"
            "+BBB\n"
        );
    }

    Y_UNIT_TEST(MultipleHunksWithContext) {
        UNIT_ASSERT_EQUAL(
            Diff(
                "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n",
                "1\n2\n3\n4\nX\n6\n7\n8\n9\n10\n11\n12\nYY\n14\n15\n16\n"
            ),
            "@@ -2,7 +2,7 @@\n"
            " 2\n"
            " 3\n"
            " 4\n"
            "-5\n"
            "+X\n"
            " 6\n"
            " 7\n"
            " 8\n"
            "@@ -10,7 +10,7 @@\n"
            " 10\n"
            " 11\n"
            " 12\n"
            "-13\n"
            "+YY\n"
            " 14\n"
            " 15\n"
            " 16\n"
        );
    }

    Y_UNIT_TEST(StressTestRepeatedLines) {
        TString left;
        for (size_t i = 0; i < 100000; i++) {
            left += "a\n";
        }
        TString right = left;
        right += "b\n";
        Diff(left, right);
    }

    Y_UNIT_TEST(EOFNewlineAdd) {
        UNIT_ASSERT_EQUAL(
            Diff("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16",
                 "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n"
            ),
            "@@ -13,4 +13,4 @@\n"
            " 13\n"
            " 14\n"
            " 15\n"
            "-16\n"
            "\\ No newline at end of file\n"
            "+16\n"
        );
    }

    Y_UNIT_TEST(EOFNewlineDel) {
        UNIT_ASSERT_EQUAL(
            Diff("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n",
                 "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16"
            ),
            "@@ -13,4 +13,4 @@\n"
            " 13\n"
            " 14\n"
            " 15\n"
            "-16\n"
            "+16\n"
            "\\ No newline at end of file\n"
        );
    }

    Y_UNIT_TEST(EOFNewlineNo) {
        UNIT_ASSERT_EQUAL(
            Diff("1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16",
                 "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n17"
            ),
            "@@ -13,4 +13,4 @@\n"
            " 13\n"
            " 14\n"
            " 15\n"
            "-16\n"
            "\\ No newline at end of file\n"
            "+17\n"
            "\\ No newline at end of file\n"
        );
    }

    Y_UNIT_TEST(IgnoreWhitespace) {
        UNIT_ASSERT_EQUAL(
            Diff("    1\n2\n",
                 "1\n    2\n",
                 3,
                 false,
                 true,
                 false,
                 false
            ),
            ""
        );
    }

    Y_UNIT_TEST(IgnoreWhitespaceWithChanges) {
        UNIT_ASSERT_EQUAL(
            Diff("    1\n2\n",
                 "1\n    3\n",
                 3,
                 false,
                 true,
                 false,
                 false
            ),
            "@@ -1,2 +1,2 @@\n"
            " 1\n"
            "-2\n"
            "+    3\n"
        );
    }

    Y_UNIT_TEST(IgnoreWhitespaceChange) {
        UNIT_ASSERT_EQUAL(
            Diff("    1\n2\n",
                 "  1\n2\n",
                 3,
                 false,
                 false,
                 true,
                 false
            ),
            ""
        );
    }

    Y_UNIT_TEST(IgnoreWhitespaceChangeWithChanges) {
        UNIT_ASSERT_EQUAL(
            Diff("    1\n2\n",
                 "  1\n3\n",
                 3,
                 false,
                 false,
                 true,
                 false
            ),
            "@@ -1,2 +1,2 @@\n"
            "   1\n"
            "-2\n"
            "+3\n"
        );
    }

    Y_UNIT_TEST(IgnoreWhitespaceEOL) {
        UNIT_ASSERT_EQUAL(
            Diff("    1\n2\n",
                 "    1  \n2  \n",
                 3,
                 false,
                 false,
                 false,
                 true
            ),
            ""
        );
    }

    Y_UNIT_TEST(IgnoreWhitespaceEOLWithChanges) {
        UNIT_ASSERT_EQUAL(
            Diff("    1\n2\n",
                 "    1  \n3  \n",
                 3,
                 false,
                 false,
                 false,
                 true
            ),
            "@@ -1,2 +1,2 @@\n"
            "     1  \n"
            "-2\n"
            "+3  \n"
        );
    }

}
