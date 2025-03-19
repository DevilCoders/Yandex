#include "dynamic_list_replacer.h"

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/strbuf.h>

#include <limits>


using namespace NFacts;


const size_t UNASSIGNED = std::numeric_limits<size_t>::max();


Y_UNIT_TEST_SUITE(FindLongestCommonSlice) {

Y_UNIT_TEST(Demo) {
    const THashVector a = {11, 12, 13, 13, 13,  1,  1,  1,  2,  3,  3,  2,  3,  3,  4, 51, 51, 52,  1,  2,  3,  4, 81, 82,  3,  3,  4, 83,};
    //                                                                 ~~~~~~~~~~~~~~
    //                      0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29
    //                                                                                                         ~~~~~~~~~~~~~~
    const THashVector b = {21, 21, 11, 22,  1,  1,  1,  1,  1, 61,  1,  2,  3,  1,  2,  4,  2,  3,  3,  3, 62,  2,  3,  3,  4,  4, 91, 81, 82, 92,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 4);
    UNIT_ASSERT_EQUAL(aToCommon, 11);
    UNIT_ASSERT_EQUAL(bToCommon, 21);
}

Y_UNIT_TEST(Empty_a0_b0) {
    const THashVector a;
    const THashVector b;

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

Y_UNIT_TEST(Empty_a0_b1) {
    const THashVector a;
    const THashVector b = {1,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

Y_UNIT_TEST(Empty_a0_b2) {
    const THashVector a;
    const THashVector b = {1, 2,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

Y_UNIT_TEST(Empty_a1_b0) {
    const THashVector a = {1,};
    const THashVector b;

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

Y_UNIT_TEST(Empty_a2_b0) {
    const THashVector a = {1, 2,};
    const THashVector b;

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

Y_UNIT_TEST(Equal_a1_b1) {
    const THashVector a = {1,};
    const THashVector b = {1,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Equal_a2_b2) {
    const THashVector a = {1, 2,};
    const THashVector b = {1, 2,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Equal_a2_b2_same_element) {
    const THashVector a = {1, 1,};
    const THashVector b = {1, 1,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Equal_zero_not_terminates) {
    const THashVector a = {0, 0,};
    const THashVector b = {0, 0,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);

}

Y_UNIT_TEST(Unequal_a1_b1) {
    const THashVector a = {1,   };
    const THashVector b = {   2,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

Y_UNIT_TEST(Unequal_a2_b2) {
    const THashVector a = {1, 2,      };
    const THashVector b = {      3, 4,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

Y_UNIT_TEST(Unequal_a1_b2) {
    const THashVector a = {1,      };
    const THashVector b = {   2, 3,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

Y_UNIT_TEST(Unequal_a2_b1) {
    const THashVector a = {1, 2,   };
    const THashVector b = {      3,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

Y_UNIT_TEST(Prefix_a1_Match_a1_b1) {
    const THashVector a = {1, 2,};
    const THashVector b = {   2,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 1);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Prefix_a1_Match_a2_b2) {
    const THashVector a = {1, 2, 3,};
    const THashVector b = {   2, 3,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 1);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Prefix_a2_Match_a1_b1) {
    const THashVector a = {1, 2, 3,};
    const THashVector b = {      3,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Prefix_a2_Match_a2_b2) {
    const THashVector a = {1, 2, 3, 4,};
    const THashVector b = {      3, 4,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Prefix_b1_Match_a1_b1) {
    const THashVector a = {   2,};
    const THashVector b = {1, 2,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 1);
}

Y_UNIT_TEST(Prefix_b1_Match_a2_b2) {
    const THashVector a = {   2, 3,};
    const THashVector b = {1, 2, 3,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 1);
}

Y_UNIT_TEST(Prefix_b2_Match_a1_b1) {
    const THashVector a = {      3,};
    const THashVector b = {1, 2, 3,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Prefix_b2_Match_a2_b2) {
    const THashVector a = {      3, 4,};
    const THashVector b = {1, 2, 3, 4,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Prefix_a2_b1_Match_a1_b1) {
    const THashVector a = {1, 2, 4,};
    const THashVector b = {   3, 4,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 1);
}

Y_UNIT_TEST(Prefix_a2_b1_Match_a2_b2) {
    const THashVector a = {1, 2, 4, 5,};
    const THashVector b = {   3, 4, 5,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 1);
}

Y_UNIT_TEST(Prefix_a1_b2_Match_a1_b1) {
    const THashVector a = {   2, 4,};
    const THashVector b = {1, 3, 4,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 1);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Prefix_a1_b2_Match_a2_b2) {
    const THashVector a = {   2, 4, 5,};
    const THashVector b = {1, 3, 4, 5,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 1);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Prefix_a2_b2_Match_a1_b1) {
    const THashVector a = {1, 3, 5,};
    const THashVector b = {2, 4, 5,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Prefix_a2_b2_Match_a2_b2) {
    const THashVector a = {1, 3, 5, 6,};
    const THashVector b = {2, 4, 5, 6,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Suffix_a1_Match_a1_b1) {
    const THashVector a = {1, 2,};
    const THashVector b = {1,   };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_a1_Match_a2_b2) {
    const THashVector a = {1, 2, 3,};
    const THashVector b = {1, 2,   };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_a2_Match_a1_b1) {
    const THashVector a = {1, 2, 3,};
    const THashVector b = {1,      };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_a2_Match_a2_b2) {
    const THashVector a = {1, 2, 3, 4,};
    const THashVector b = {1, 2,      };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_b1_Match_a1_b1) {
    const THashVector a = {1,   };
    const THashVector b = {1, 2,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_b1_Match_a2_b2) {
    const THashVector a = {1, 2,   };
    const THashVector b = {1, 2, 3,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_b2_Match_a1_b1) {
    const THashVector a = {1,      };
    const THashVector b = {1, 2, 3,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_b2_Match_a2_b2) {
    const THashVector a = {1, 2,      };
    const THashVector b = {1, 2, 3, 4,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_a2_b1_Match_a1_b1) {
    const THashVector a = {1, 2, 4,};
    const THashVector b = {1, 3,   };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_a2_b1_Match_a2_b2) {
    const THashVector a = {1, 2, 3, 5,};
    const THashVector b = {1, 2, 4,   };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_a1_b2_Match_a1_b1) {
    const THashVector a = {1, 2,   };
    const THashVector b = {1, 3, 4,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_a1_b2_Match_a2_b2) {
    const THashVector a = {1, 2, 3,   };
    const THashVector b = {1, 2, 4, 5,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_a2_b2_Match_a1_b1) {
    const THashVector a = {1, 2, 4,};
    const THashVector b = {1, 3, 5,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 1);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Suffix_a2_b2_Match_a2_b2) {
    const THashVector a = {1, 2, 3, 5,};
    const THashVector b = {1, 2, 4, 6,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Affixes_a2a2_b0b0) {
    const THashVector a = {1, 2, 3, 4, 5, 6,};
    const THashVector b = {      3, 4,      };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Affixes_a0a0_b2b2) {
    const THashVector a = {      3, 4,      };
    const THashVector b = {1, 2, 3, 4, 5, 6,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Affixes_a2a0_b0b2) {
    const THashVector a = {1, 2, 3, 4,      };
    const THashVector b = {      3, 4, 5, 6,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Affixes_a0a2_b2b0) {
    const THashVector a = {      3, 4, 5, 6,};
    const THashVector b = {1, 2, 3, 4,      };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Affixes_a2a2_b2b0) {
    const THashVector a = {1, 3, 5, 6, 7, 8,};
    const THashVector b = {2, 4, 5, 6,      };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Affixes_a2a0_b2b2) {
    const THashVector a = {1, 3, 5, 6,      };
    const THashVector b = {2, 4, 5, 6, 7, 8,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Affixes_a2a2_b0b2) {
    const THashVector a = {1, 2, 3, 4, 5, 7,};
    const THashVector b = {      3, 4, 6, 8,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(Affixes_a0a2_b2b2) {
    const THashVector a = {      3, 4, 5, 7,};
    const THashVector b = {1, 2, 3, 4, 6, 8,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 0);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Affixes_a2a2_b2b2) {
    const THashVector a = {1, 3, 5, 6, 7, 9,};
    const THashVector b = {2, 4, 5, 6, 8, 0,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Affixes_a2a2_b2b1) {
    const THashVector a = {1, 3, 5, 6, 7, 9,};
    const THashVector b = {2, 4, 5, 6, 8,   };

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Affixes_a2a1_b2b2) {
    const THashVector a = {1, 3, 5, 6, 7,   };
    const THashVector b = {2, 4, 5, 6, 8, 9,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Affixes_a2a2_b1b2) {
    const THashVector a = {1, 2, 4, 5, 6, 8,};
    const THashVector b = {   3, 4, 5, 7, 9,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 1);
}

Y_UNIT_TEST(Affixes_a1a2_b2b2) {
    const THashVector a = {   2, 4, 5, 6, 8,};
    const THashVector b = {1, 3, 4, 5, 7, 9,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 1);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Repeated_A_no_overlap) {
    const THashVector a = {1, 3, 5, 6, 5, 6, 5, 6, 7, 9,};
    const THashVector b = {2, 4, 5, 6,             8, 0,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT(aToCommon == 2 || aToCommon == 4 || aToCommon == 6);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Repeated_B_no_overlap) {
    const THashVector a = {2, 4, 5, 6,             8, 0,};
    const THashVector b = {1, 3, 5, 6, 5, 6, 5, 6, 7, 9,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 2);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT(bToCommon == 2 || bToCommon == 4 || bToCommon == 6);
}

Y_UNIT_TEST(Repeated_A_overlap) {
    const THashVector a = {1, 3, 5, 6, 5, 6, 5, 6, 7, 9,};
    const THashVector b = {2, 4, 5, 6, 5, 6,       8, 0,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 4);
    UNIT_ASSERT(aToCommon == 2 || aToCommon == 4);
    UNIT_ASSERT_EQUAL(bToCommon, 2);
}

Y_UNIT_TEST(Repeated_B_overlap) {
    const THashVector a = {1, 3, 5, 6, 5, 6,       7, 9,};
    const THashVector b = {2, 4, 5, 6, 5, 6, 5, 6, 8, 0,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon);

    UNIT_ASSERT_EQUAL(longestCommonSize, 4);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT(bToCommon == 2 || bToCommon == 4);
}

Y_UNIT_TEST(MinAllowedCommonSliceSize_2) {
    const THashVector a = {1, 2, 3, 4, 5,      };
    const THashVector b = {      3, 4, 5, 6, 7,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon, /*minAllowedCommonSliceSize*/ 2);

    UNIT_ASSERT_EQUAL(longestCommonSize, 3);
    UNIT_ASSERT_EQUAL(aToCommon, 2);
    UNIT_ASSERT_EQUAL(bToCommon, 0);
}

Y_UNIT_TEST(MinAllowedCommonSliceSize_4) {
    const THashVector a = {1, 2, 3, 4, 5,      };
    const THashVector b = {      3, 4, 5, 6, 7,};

    size_t aToCommon = UNASSIGNED;
    size_t bToCommon = UNASSIGNED;
    size_t longestCommonSize = FindLongestCommonSlice(a, b, aToCommon, bToCommon, /*minAllowedCommonSliceSize*/ 4);

    UNIT_ASSERT_EQUAL(longestCommonSize, 0);
    UNIT_ASSERT_EQUAL(aToCommon, UNASSIGNED);
    UNIT_ASSERT_EQUAL(bToCommon, UNASSIGNED);
}

}  // FindLongestCommonSlice


Y_UNIT_TEST_SUITE(MatchFactTextWithListCandidate) {

Y_UNIT_TEST(ComplexDemo) {
    const TString factText = "К таким привычкам относятся: сидеть или ходить, сгорбив спину; привычка грызть ноги...";

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = "092AnRa8ttEnOKe2bsda5cALa8Sf/+Lzh/zuYmAZBLCJYYuVpSdRKRDa96WiBwbN7mIjnZD/UWEhatTbnKtI8BKdKbKIi0YbXPOHEr/mEk4+zV8yS8G/5i0YXfkGLGxF7jZIM1B2aI9t+d0Dv+bQx4cqUx6iBwHNXZY=";
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll({ 29, 4, });
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll({ 5, 3, 2, 5, 1, 1, 1, 3, 7, });

    THashVector factTextHash;
    UNIT_ASSERT_NO_EXCEPTION([&](){ factTextHash = DynamicListReplacerHashVector(factText); }());

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 12);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(ComplexDemoJson) {
    const TString factText = "К таким привычкам относятся: сидеть или ходить, сгорбив спину; привычка грызть ноги...";

    const TString listCandidateIndexJson =
        R"ELEPHANT(
            {
                "body_hash": "092AnRa8ttEnOKe2bsda5cALa8Sf/+Lzh/zuYmAZBLCJYYuVpSdRKRDa96WiBwbN7mIjnZD/UWEhatTbnKtI8BKdKbKIi0YbXPOHEr/mEk4+zV8yS8G/5i0YXfkGLGxF7jZIM1B2aI9t+d0Dv+bQx4cqUx6iBwHNXZY=",
                "header_lens": [29, 4],
                "items_lens": [5, 3, 2, 5, 1, 1, 1, 3, 7]
            }
        )ELEPHANT";

    THashVector factTextHash;
    UNIT_ASSERT_NO_EXCEPTION([&](){ factTextHash = DynamicListReplacerHashVector(factText); }());

    TMatchResult matchResult;
    UNIT_ASSERT_NO_EXCEPTION([&](){ matchResult = MatchFactTextWithListCandidate(factTextHash, listCandidateIndexJson); }());

    UNIT_ASSERT_EQUAL(matchResult.ListCandidateScore, 12);
    UNIT_ASSERT_EQUAL(matchResult.NumberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(Demo_Extend_fact_text) {
    const THashVector factTextHash =      {                 5,  6,  7,  8,   11, 12, 13, 14, 15, 16,   21, 22, 23, 24,                                                    };
    //                                      ............. list header end    first item............    second item start.....
    //                                      full list header.............    first item............    full second item......    third item............    fourth item...
    const THashVector listCandidateHash = { 1,  2,  3,  4,  5,  6,  7,  8,   11, 12, 13, 14, 15, 16,   21, 22, 23, 24, 25, 26,   31, 32, 33, 34, 35, 36,   41, 42, 43, 44,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll({
        2,  // Not found in the fact text.
        6,  // Matched with the biginning of the fact text for 4/6.
    });
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll({
        6,  // Fully contained in the fact text.
        6,  // Matched with the end of the fact text for 4/6.
        6,  // Not found in the fact text.
        4,  // Not found in the fact text.
    });

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 14);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(Demo_Cut_fact_text) {
    const THashVector factTextHash =      {91, 92, 93, 94, 95,   1,  2,  3,  4,  5,  6,  7,  8,   11, 12, 13, 14, 15, 16,   21, 22, 23, 24, 25, 26,   31, 32, 33, 34, 35, 36,   41, 42, 43, 44,   96, 97, 98, 99,    };
    //                                     leading extra text    list header..................    first item............    second item...........    third item............    fourth item...    training extra text
    //                                                           list header..................    first item............    second item...........    third item............    fourth item...
    const THashVector listCandidateHash = {                      1,  2,  3,  4,  5,  6,  7,  8,   11, 12, 13, 14, 15, 16,   21, 22, 23, 24, 25, 26,   31, 32, 33, 34, 35, 36,   41, 42, 43, 44,                      };

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll({
        2,  // Fully contained in the fact text.
        6,  // Fully contained in the fact text.
    });
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll({
        6,  // Fully contained in the fact text.
        6,  // Fully contained in the fact text.
        6,  // Fully contained in the fact text.
        4,  // Fully contained in the fact text.
    });

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 30);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Demo_Cut_and_extend_fact_text) {
    const THashVector factTextHash =      {91, 92, 93, 94, 95,   1,  2,  3,  4,  5,  6,  7,  8,   11, 12, 13, 14, 15, 16,   21, 22, 23, 24,                                                    };
    //                                     leading extra text    list header..................    first item............    second item start.....
    //                                                           list header..................    first item............    full second item......    third item............    fourth item...
    const THashVector listCandidateHash = {                      1,  2,  3,  4,  5,  6,  7,  8,   11, 12, 13, 14, 15, 16,   21, 22, 23, 24, 25, 26,   31, 32, 33, 34, 35, 36,   41, 42, 43, 44,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll({
        2,  // Fully contained in the fact text.
        6,  // Fully contained in the fact text.
    });
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll({
        6,  // Fully contained in the fact text.
        6,  // Matched with the end of the fact text for 4/6.
        6,  // Not found in the fact text.
        4,  // Not found in the fact text.
    });

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 18);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Demo_Extend_and_cut_fact_text) {
    const THashVector factTextHash =      {                 5,  6,  7,  8,   11, 12, 13, 14, 15, 16,   21, 22, 23, 24, 25, 26,   31, 32, 33, 34, 35, 36,   41, 42, 43, 44,   96, 97, 98, 99,    };
    //                                      ............. list header end    first item............    second item...........    third item............    fourth item...    training extra text
    //                                      full list header.............    first item............    second item...........    third item............    fourth item...
    const THashVector listCandidateHash = { 1,  2,  3,  4,  5,  6,  7,  8,   11, 12, 13, 14, 15, 16,   21, 22, 23, 24, 25, 26,   31, 32, 33, 34, 35, 36,   41, 42, 43, 44,                      };

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll({
        2,  // Not found in the fact text.
        6,  // Matched with the biginning of the fact text for 4/6.
    });
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll({
        6,   // Fully contained in the fact text.
        6,   // Fully contained in the fact text.
        6,   // Fully contained in the fact text.
        4,   // Fully contained in the fact text.
    });

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 26);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(Exception_Base64Decode_too_short) {
    const THashVector factTextHash;
    const THashVector listCandidateHash = {0, 1, 2,};

    TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    bodyHash.resize(bodyHash.size() - 1);

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    UNIT_ASSERT_EXCEPTION(MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip), yexception);
}

Y_UNIT_TEST(Exception_Base64Decode__too_long) {
    const THashVector factTextHash;
    const THashVector listCandidateHash = {0, 1, 2,};

    TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    bodyHash.resize(bodyHash.size() + 1);

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    UNIT_ASSERT_EXCEPTION(MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip), yexception);
}

Y_UNIT_TEST(Exception_Invalid_header_lens_value) {
    const THashVector factTextHash      = {0, 1, 2,};
    const THashVector listCandidateHash = {0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {-1,}   );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    UNIT_ASSERT_EXCEPTION_CONTAINS(MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip), yexception, "Invalid header_lens value");
}

Y_UNIT_TEST(Exception_Invalid_items_lens_value_zero) {
    const THashVector factTextHash      = {0, 1, 2,};
    const THashVector listCandidateHash = {0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 0,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    UNIT_ASSERT_EXCEPTION_CONTAINS(MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip), yexception, "Invalid items_lens value");
}

Y_UNIT_TEST(Exception_Invalid_items_lens_value_negative) {
    const THashVector factTextHash      = {0, 1, 2,};
    const THashVector listCandidateHash = {0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}     );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, -1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    UNIT_ASSERT_EXCEPTION_CONTAINS(MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip), yexception, "Invalid items_lens value");
}

Y_UNIT_TEST(Exception_Not_enough_header_elements_described) {
    const THashVector factTextHash      = {1, 2,};
    const THashVector listCandidateHash = {1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].SetArray();
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    UNIT_ASSERT_EXCEPTION_CONTAINS(MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip), yexception, "Not enough header elements described");
}

Y_UNIT_TEST(Exception_Not_enough_list_items_described) {
    const THashVector factTextHash      = {0, 1,};
    const THashVector listCandidateHash = {0, 1,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    UNIT_ASSERT_EXCEPTION_CONTAINS(MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip), yexception, "Not enough list items described");
}

Y_UNIT_TEST(Exception_Invalid_body_hash_length) {
    const THashVector factTextHash      = {0, 1, 2,};
    const THashVector listCandidateHash = {0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 2,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    UNIT_ASSERT_EXCEPTION_CONTAINS(MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip), yexception, "Invalid body_hash length");
}

Y_UNIT_TEST(No_match_Empty_common_slice) {
    const THashVector factTextHash      = {7, 8, 9};
    const THashVector listCandidateHash = {0, 1, 2};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 0);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, UNASSIGNED);
}

Y_UNIT_TEST(No_match_Too_short_common_slice) {
    const THashVector factTextHash      = {9, 9, 9, 9, 0, 1, 2,};
    const THashVector listCandidateHash = {            0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 0);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, UNASSIGNED);
}

Y_UNIT_TEST(No_match_Different_leading_extra_text) {
    const THashVector factTextHash      = {9, 0, 1, 2,};
    const THashVector listCandidateHash = {0, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 0);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, UNASSIGNED);
}

Y_UNIT_TEST(No_match_Different_trailing_extra_text) {
    const THashVector factTextHash      = {0, 1, 2, 9,};
    const THashVector listCandidateHash = {0, 1, 2, 3,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}       );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 0);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, UNASSIGNED);
}

Y_UNIT_TEST(No_match_First_item_not_fully_included) {
    const THashVector factTextHash      = {0, 1,      };
    const THashVector listCandidateHash = {0, 1, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {2, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 0);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, UNASSIGNED);
}

Y_UNIT_TEST(No_match_Too_short_part_of_second_item_included) {
    const THashVector factTextHash      = {0, 1, 2,      };
    const THashVector listCandidateHash = {0, 1, 2, 2, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 3,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 0);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, UNASSIGNED);
}

Y_UNIT_TEST(Match_Without_header_1) {
    const THashVector factTextHash      = {   1, 2,};
    const THashVector listCandidateHash = {0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 2);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_Without_header_2) {
    const THashVector factTextHash      = {      1, 2,};
    const THashVector listCandidateHash = {0, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 2);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(Match_Without_half_of_second_item) {
    const THashVector factTextHash      = {0, 1, 2,   };
    const THashVector listCandidateHash = {0, 1, 2, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 2,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_Without_third_item) {
    const THashVector factTextHash      = {0, 1, 2,   };
    const THashVector listCandidateHash = {0, 1, 2, 3,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1,}       );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_Too_short_header_1) {
    const THashVector factTextHash      = {      0, 1, 2,};
    const THashVector listCandidateHash = {0, 0, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {3,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_Too_short_header_2) {
    const THashVector factTextHash      = {      9, 0, 1, 2,};
    const THashVector listCandidateHash = {9, 9, 9, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {3, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 4);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(Match_Without_half_of_header_1) {
    const THashVector factTextHash      = {   0, 1, 2,};
    const THashVector listCandidateHash = {0, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {2,}    );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_Without_half_of_header_2) {
    const THashVector factTextHash      = {   9, 0, 1, 2,};
    const THashVector listCandidateHash = {9, 9, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {2, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 4);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_With_empty_header_h1h0) {
    const THashVector factTextHash      = {0,       1, 2,};
    const THashVector listCandidateHash = {0, /*,*/ 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 0,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_With_empty_header_h1h0h0) {
    const THashVector factTextHash      = {0,         1, 2,};
    const THashVector listCandidateHash = {0, /*, ,*/ 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 0, 0,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}    );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_With_empty_header_h1h1h0) {
    const THashVector factTextHash      = {9, 0,       1, 2,};
    const THashVector listCandidateHash = {9, 0, /*,*/ 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 1, 0,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}    );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 4);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_With_empty_header_h1h0h1) {
    const THashVector factTextHash      = {9,       0, 1, 2,};
    const THashVector listCandidateHash = {9, /*,*/ 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 0, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}    );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 4);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_With_empty_header_h1h0h0h1) {
    const THashVector factTextHash      = {9,         0, 1, 2,};
    const THashVector listCandidateHash = {9, /*, ,*/ 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 0, 0, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}       );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 4);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_Skip_header_h1h1) {
    const THashVector factTextHash      = {   0, 1, 2,};
    const THashVector listCandidateHash = {9, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(Match_Skip_header_h2h1) {
    const THashVector factTextHash      = {      0, 1, 2,};
    const THashVector listCandidateHash = {9, 9, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {2, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(Match_Skip_header_h1h2) {
    const THashVector factTextHash      = {   0, 0, 1, 2,};
    const THashVector listCandidateHash = {9, 0, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 2,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 4);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(Match_Skip_header_h1h1h1) {
    const THashVector factTextHash      = {      0, 1, 2,};
    const THashVector listCandidateHash = {8, 9, 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 1, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}    );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 2);
}

Y_UNIT_TEST(Match_Skip_header_h0h1) {
    const THashVector factTextHash      = {      0, 1, 2,};
    const THashVector listCandidateHash = {/*,*/ 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {0, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

Y_UNIT_TEST(Match_Skip_header_h0h0h1) {
    const THashVector factTextHash      = {        0, 1, 2,};
    const THashVector listCandidateHash = {/*, ,*/ 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {0, 0, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}    );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 2);
}

Y_UNIT_TEST(Match_Skip_header_h1h0h1) {
    const THashVector factTextHash      = {         0, 1, 2,};
    const THashVector listCandidateHash = {9, /*,*/ 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 0, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}    );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 2);
}

Y_UNIT_TEST(Match_Skip_header_h1h0h0h1) {
    const THashVector factTextHash      = {           0, 1, 2,};
    const THashVector listCandidateHash = {9, /*, ,*/ 0, 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 0, 0, 1,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}       );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 3);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 3);
}

Y_UNIT_TEST(Match_Skip_header_h1h0) {
    const THashVector factTextHash      = {         1, 2,};
    const THashVector listCandidateHash = {0, /*,*/ 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 0,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,} );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 2);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_Skip_header_h1h0h0) {
    const THashVector factTextHash      = {           1, 2,};
    const THashVector listCandidateHash = {0, /*, ,*/ 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 0, 0,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}    );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 2);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 0);
}

Y_UNIT_TEST(Match_Skip_header_h1h1h0) {
    const THashVector factTextHash      = {            1, 2,};
    const THashVector listCandidateHash = {9, 0, /*,*/ 1, 2,};

    const TString bodyHash = Base64Encode(TStringBuf(reinterpret_cast<const char*>(listCandidateHash.data()), listCandidateHash.size() * 2));

    NSc::TValue listCandidateIndex;
    listCandidateIndex["body_hash"] = bodyHash;
    listCandidateIndex["header_lens"].GetArrayMutable().AppendAll( {1, 1, 0,} );
    listCandidateIndex["items_lens"] .GetArrayMutable().AppendAll( {1, 1,}    );

    size_t numberOfHeaderElementsToSkip = UNASSIGNED;
    size_t listCandidateScore = UNASSIGNED;
    UNIT_ASSERT_NO_EXCEPTION([&](){ listCandidateScore = MatchFactTextWithListCandidate(factTextHash, listCandidateIndex.GetDict(), numberOfHeaderElementsToSkip); }());

    UNIT_ASSERT_EQUAL(listCandidateScore, 2);
    UNIT_ASSERT_EQUAL(numberOfHeaderElementsToSkip, 1);
}

}  // MatchFactTextWithListCandidate


Y_UNIT_TEST_SUITE(ConvertListDataToRichFact) {

Y_UNIT_TEST(ComplexDemo) {
    NSc::TValue serpData;
    serpData["url"] = "http://03hm.ru/info/news/priroda-vozniknoveniya-vrednykh-privychek/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["type"] = 0;
    listData["header"].GetArrayMutable().AppendAll({
        "Вредные привычки имеют также обширный перечень, который знаком каждому с ранних лет, потому что очень часто повторяют мамы, бабушки, воспитатели детского сада и учителя, что нельзя делать некоторые вещи.",
        "К таким привычкам относятся:",
    });
    listData["items"].GetArrayMutable().AppendAll({
        "сидеть или ходить, сгорбив спину",
        "привычка грызть ноги",
        "«щелканье» суставами",
        "привычка питаться едой быстрого приготовления",
        "курение",
        "алкоголь",
        "наркотики",
        "неумеренность в еде",
        "привычка поздно ложиться спать",
    });

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["type"] = "rich_fact";
    serpDataExpected["use_this_type"] = 1;
    {
        NSc::TValue& visibleItemContent = serpDataExpected["visible_items"][0]["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "Вредные привычки имеют также обширный перечень, который знаком каждому с ранних лет, потому что очень часто повторяют мамы, бабушки, воспитатели детского сада и учителя, что нельзя делать некоторые вещи. К таким привычкам относятся:";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][1];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "сидеть или ходить, сгорбив спину";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][2];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "привычка грызть ноги";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][3];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "«щелканье» суставами";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][4];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "привычка питаться едой быстрого приготовления";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][5];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "курение";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][6];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "алкоголь";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][7];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "наркотики";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][8];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "неумеренность в еде";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][9];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "привычка поздно ложиться спать";
    }

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_NO_EXCEPTION(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 0));

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(ComplexDemoJson) {
    const TString serpDataInJson =
        R"ELEPHANT(
            {
                "source": "fact_snippet",
                "type": "suggest_fact",
                "url": "http://03hm.ru/info/news/priroda-vozniknoveniya-vrednykh-privychek/"
            }
        )ELEPHANT";

    const TString listDataJson =
        R"ELEPHANT(
            {
                "header": [
                    "Вредные привычки имеют также обширный перечень, который знаком каждому с ранних лет, потому что очень часто повторяют мамы, бабушки, воспитатели детского сада и учителя, что нельзя делать некоторые вещи.",
                    "К таким привычкам относятся:"
                ],
                "items": [
                    "сидеть или ходить, сгорбив спину",
                    "привычка грызть ноги",
                    "«щелканье» суставами",
                    "привычка питаться едой быстрого приготовления",
                    "курение",
                    "алкоголь",
                    "наркотики",
                    "неумеренность в еде",
                    "привычка поздно ложиться спать"
                ],
                "type": 0
            }
        )ELEPHANT";

    const TString serpDataExpectedJson =
        R"ELEPHANT(
            {
                "source": "fact_snippet",
                "type": "rich_fact",
                "use_this_type": 1,
                "url": "http://03hm.ru/info/news/priroda-vozniknoveniya-vrednykh-privychek/",
                "visible_items": [
                    {
                        "content": [
                            {
                                "type": "text",
                                "text": "Вредные привычки имеют также обширный перечень, который знаком каждому с ранних лет, потому что очень часто повторяют мамы, бабушки, воспитатели детского сада и учителя, что нельзя делать некоторые вещи. К таким привычкам относятся:"
                            }
                        ]
                    },
                    {
                        "marker": "bullet",
                        "content": [
                            {
                                "type": "text",
                                "text": "сидеть или ходить, сгорбив спину"
                            }
                        ]
                    },
                    {
                        "marker": "bullet",
                        "content": [
                            {
                                "type": "text",
                                "text": "привычка грызть ноги"
                            }
                        ]
                    },
                    {
                        "marker": "bullet",
                        "content": [
                            {
                                "type": "text",
                                "text": "«щелканье» суставами"
                            }
                        ]
                    },
                    {
                        "marker": "bullet",
                        "content": [
                            {
                                "type": "text",
                                "text": "привычка питаться едой быстрого приготовления"
                            }
                        ]
                    },
                    {
                        "marker": "bullet",
                        "content": [
                            {
                                "type": "text",
                                "text": "курение"
                            }
                        ]
                    },
                    {
                        "marker": "bullet",
                        "content": [
                            {
                                "type": "text",
                                "text": "алкоголь"
                            }
                        ]
                    },
                    {
                        "marker": "bullet",
                        "content": [
                            {
                                "type": "text",
                                "text": "наркотики"
                            }
                        ]
                    },
                    {
                        "marker": "bullet",
                        "content": [
                            {
                                "type": "text",
                                "text": "неумеренность в еде"
                            }
                        ]
                    },
                    {
                        "marker": "bullet",
                        "content": [
                            {
                                "type": "text",
                                "text": "привычка поздно ложиться спать"
                            }
                        ]
                    }
                ]
            }
        )ELEPHANT";

    TString serpDataOutJson;
    UNIT_ASSERT_NO_EXCEPTION([&](){ serpDataOutJson = ConvertListDataToRichFact(serpDataInJson, listDataJson, /*numberOfHeaderElementsToSkip*/ 0); }());

    const NSc::TJsonOpts jsonOptsStrict = NSc::TJsonOpts(
            NSc::TJsonOpts::JO_PARSER_STRICT_JSON |
            NSc::TJsonOpts::JO_PARSER_STRICT_UTF8 |
            NSc::TJsonOpts::JO_PARSER_DISALLOW_COMMENTS |
            NSc::TJsonOpts::JO_PARSER_DISALLOW_DUPLICATE_KEYS);

    NSc::TValue serpDataOut;
    UNIT_ASSERT_NO_EXCEPTION([&](){ serpDataOut = NSc::TValue::FromJsonThrow(serpDataOutJson, jsonOptsStrict); }());

    NSc::TValue serpDataExpected;
    UNIT_ASSERT_NO_EXCEPTION([&](){ serpDataExpected = NSc::TValue::FromJsonThrow(serpDataExpectedJson, jsonOptsStrict); }());

    UNIT_ASSERT(NSc::TValue::Equal(serpDataOut, serpDataExpected));
}

Y_UNIT_TEST(Unordered_list_by_default) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["header"].GetArrayMutable().AppendAll( {"ab", "cd ef",}    );
    listData["items"] .GetArrayMutable().AppendAll( {"gh", "ij", "kl",} );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["type"] = "rich_fact";
    serpDataExpected["use_this_type"] = 1;
    {
        NSc::TValue& visibleItemContent = serpDataExpected["visible_items"][0]["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ab cd ef";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][1];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "gh";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][2];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ij";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][3];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "kl";
    }

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_NO_EXCEPTION(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 0));

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(Unordered_list) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["type"] = 0;
    listData["header"].GetArrayMutable().AppendAll( {"ab", "cd ef",}    );
    listData["items"] .GetArrayMutable().AppendAll( {"gh", "ij", "kl",} );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["type"] = "rich_fact";
    serpDataExpected["use_this_type"] = 1;
    {
        NSc::TValue& visibleItemContent = serpDataExpected["visible_items"][0]["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ab cd ef";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][1];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "gh";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][2];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ij";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][3];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "kl";
    }

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_NO_EXCEPTION(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 0));

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(Ordered_list) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["type"] = 1;
    listData["header"].GetArrayMutable().AppendAll( {"ab", "cd ef",}    );
    listData["items"] .GetArrayMutable().AppendAll( {"gh", "ij", "kl",} );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["type"] = "rich_fact";
    serpDataExpected["use_this_type"] = 1;
    {
        NSc::TValue& visibleItemContent = serpDataExpected["visible_items"][0]["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ab cd ef";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][1];
        visibleItem["marker"] = 1;
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "gh";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][2];
        visibleItem["marker"] = 2;
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ij";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][3];
        visibleItem["marker"] = 3;
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "kl";
    }

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_NO_EXCEPTION(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 0));

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(Skip_header_1_of_2) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["header"].GetArrayMutable().AppendAll( {"ab", "cd ef",}    );
    listData["items"] .GetArrayMutable().AppendAll( {"gh", "ij", "kl",} );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["type"] = "rich_fact";
    serpDataExpected["use_this_type"] = 1;
    {
        NSc::TValue& visibleItemContent = serpDataExpected["visible_items"][0]["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "cd ef";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][1];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "gh";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][2];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ij";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][3];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "kl";
    }

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_NO_EXCEPTION(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 1));

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(Skip_header_1_of_3) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["header"].GetArrayMutable().AppendAll( {"ab", "cd", "ef",} );
    listData["items"] .GetArrayMutable().AppendAll( {"gh", "ij", "kl",} );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["type"] = "rich_fact";
    serpDataExpected["use_this_type"] = 1;
    {
        NSc::TValue& visibleItemContent = serpDataExpected["visible_items"][0]["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "cd ef";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][1];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "gh";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][2];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ij";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][3];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "kl";
    }

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_NO_EXCEPTION(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 1));

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(Skip_header_2_of_3) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["header"].GetArrayMutable().AppendAll( {"ab", "cd", "ef",} );
    listData["items"] .GetArrayMutable().AppendAll( {"gh", "ij", "kl",} );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["type"] = "rich_fact";
    serpDataExpected["use_this_type"] = 1;
    {
        NSc::TValue& visibleItemContent = serpDataExpected["visible_items"][0]["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ef";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][1];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "gh";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][2];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "ij";
    }
    {
        NSc::TValue& visibleItem = serpDataExpected["visible_items"][3];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "kl";
    }

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_NO_EXCEPTION(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 2));

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(Exception_Fact_is_already_rich) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "rich_fact";
    serpData["source"] = "fact_snippet";
    serpData["use_this_type"] = 1;
    {
        NSc::TValue& visibleItemContent = serpData["visible_items"][0]["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "1 2 3";
    }
    {
        NSc::TValue& visibleItem = serpData["visible_items"][1];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "4 5";
    }
    {
        NSc::TValue& visibleItem = serpData["visible_items"][2];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "6 7";
    }
    {
        NSc::TValue& visibleItem = serpData["visible_items"][3];
        visibleItem["marker"] = "bullet";
        NSc::TValue& visibleItemContent = visibleItem["content"][0];
        visibleItemContent["type"] = "text";
        visibleItemContent["text"] = "8 9";
    }

    NSc::TValue listData;
    listData["header"].GetArrayMutable().AppendAll( {"ab", "cd ef",}    );
    listData["items"] .GetArrayMutable().AppendAll( {"gh", "ij", "kl",} );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["_tag_"] = "_tag_";

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_EXCEPTION_CONTAINS(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 0), yexception, "Fact is already rich");

    serpData["_tag_"] = "_tag_";

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(Exception_Cannot_enrich_with_empty_list_header_0) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["header"].SetArray();
    listData["items"] .GetArrayMutable().AppendAll( {"gh", "ij", "kl",} );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["_tag_"] = "_tag_";

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_EXCEPTION_CONTAINS(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 0), yexception, "Cannot enrich with empty list header");

    serpData.Delete("visible_items");
    serpData["_tag_"] = "_tag_";

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(Exception_Cannot_enrich_with_empty_list_header_2) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["header"].GetArrayMutable().AppendAll( {"ab", "cd ef",}    );
    listData["items"] .GetArrayMutable().AppendAll( {"gh", "ij", "kl",} );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["_tag_"] = "_tag_";

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_EXCEPTION_CONTAINS(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 2), yexception, "Cannot enrich with empty list header");

    serpData.Delete("visible_items");
    serpData["_tag_"] = "_tag_";

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

Y_UNIT_TEST(Exception_Cannot_enrich_with_less_than_two_list_items) {
    NSc::TValue serpData;
    serpData["url"] = "https://my.host.ru/";
    serpData["type"] = "suggest_fact";
    serpData["source"] = "fact_snippet";

    NSc::TValue listData;
    listData["header"].GetArrayMutable().AppendAll( {"ab", "cd ef",} );
    listData["items"] .GetArrayMutable().AppendAll( {"gh",}          );

    NSc::TValue serpDataExpected = serpData.Clone();
    serpDataExpected["_tag_"] = "_tag_";

    UNIT_ASSERT(!NSc::TValue::Equal(serpData, serpDataExpected));

    UNIT_ASSERT_EXCEPTION_CONTAINS(ConvertListDataToRichFact(serpData, listData, /*numberOfHeaderElementsToSkip*/ 0), yexception, "Cannot enrich with less than two list items");

    serpData.Delete("visible_items");
    serpData["_tag_"] = "_tag_";

    UNIT_ASSERT(NSc::TValue::Equal(serpData, serpDataExpected));
}

}  // ConvertListDataToRichFact


Y_UNIT_TEST_SUITE(NormalizeTextForIndexer) {

Y_UNIT_TEST(KeepAlmunRemovePunct) {
    const TUtf16String text = u"some text, another text - 123.45/678 больше текстов!!!";
    const TUtf16String norm = u"some text another text 123 45 678 больше текстов";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(ConvertCapitalToSmall) {
    const TUtf16String text = u"Some TEXT Некий ТЕКСТ";
    const TUtf16String norm = u"some text некий текст";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(ShrinkSpaces) {
    const TUtf16String text = u"  some  text с        пробелами   ";
    const TUtf16String norm = u"some text с пробелами";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(RemoveSoftHyphen) {
    const TUtf16String text = u"пере\u00ADнос 123\u00AD456 a[\u00AD2] \u00AD xxx";
    const TUtf16String norm = u"перенос 123456 a 2 xxx";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(RemoveCombiningAcuteAccent) {
    const TUtf16String text = u"ударе\u0301ние 123\u0301456 a[\u03012] \u0301 xxx";
    const TUtf16String norm = u"ударение 123456 a 2 xxx";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(ReplaceIoWithIe) {
    const TUtf16String text = u"ЕЩЁ ещё";
    const TUtf16String norm = u"еще еще";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(RemoveEmoticons) {
    const TUtf16String text = u"смайлик ☺ удаляем";
    const TUtf16String norm = u"смайлик удаляем";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(KeepHieroglyphs) {
    const TUtf16String text = u"иероглиф 呆 сораняем";
    const TUtf16String norm = u"иероглиф 呆 сораняем";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(Empty) {
    const TUtf16String text = u"  ";
    const TUtf16String norm = u"";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(ShrinkSpacesToEmpty) {
    const TUtf16String text = u"  ";
    const TUtf16String norm = u"";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(RemovePunctToEmpty) {
    const TUtf16String text = u". . .";
    const TUtf16String norm = u"";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(MinusSign) {
    const TUtf16String text = u"-1 - 2 = -3";
    const TUtf16String norm = u"1 2 3";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(MinusWord_1) {
    const TUtf16String text = u"залоченный это -айфон";
    const TUtf16String norm = u"залоченный это айфон";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(MinusWord_2) {
    const TUtf16String text = u"залоченный это -айфон-телефон";
    const TUtf16String norm = u"залоченный это айфон телефон";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(MinusWord_3) {
    const TUtf16String text = u"залоченный это -айфон телефон";
    const TUtf16String norm = u"залоченный это айфон телефон";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(MinusWord_4) {
    const TUtf16String text = u"залоченный это -айфон -телефон";
    const TUtf16String norm = u"залоченный это айфон телефон";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(MinusWord_5) {
    const TUtf16String text = u"залоченный это - айфон-телефон";
    const TUtf16String norm = u"залоченный это айфон телефон";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(ComplexText) {
    const TUtf16String text = u"  Какой-нибудь  текст (со скобочками) и ударе\u0301нием, и пере\u00ADносом, и буквой Ёё, и смайликом ☺, и иероглифом 呆 -- чтобы !!!ПРОВЕРИТЬ!!!, \n\r А) \r\n Б) \n В) \r Г) ... \"\' как он тут <{[123.45]}> нормализуется...    ";
    const TUtf16String norm = u"какой нибудь текст со скобочками и ударением и переносом и буквой ее и смайликом и иероглифом 呆 чтобы проверить а б в г как он тут 123 45 нормализуется";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

Y_UNIT_TEST(ComplexTextUtf8) {
    const TString text = "  Какой-нибудь  текст (со скобочками) и ударе\u0301нием, и пере\u00ADносом, и буквой Ёё, и смайликом ☺, и иероглифом 呆 -- чтобы !!!ПРОВЕРИТЬ!!!, \n\r А) \r\n Б) \n В) \r Г) ... \"\' как он тут <{[123.45]}> нормализуется...    ";
    const TString norm = "какой нибудь текст со скобочками и ударением и переносом и буквой ее и смайликом и иероглифом 呆 чтобы проверить а б в г как он тут 123 45 нормализуется";

    UNIT_ASSERT_EQUAL(NormalizeTextForIndexer(text), norm);
}

}  // NormalizeTextForIndexer


Y_UNIT_TEST_SUITE(DynamicListReplacerHashVector) {

Y_UNIT_TEST(EmptyText) {
    const TUtf16String text = u". . .";

    const THashVector hashVectorExpected;  // empty

    const THashVector hashVector = DynamicListReplacerHashVector(text);

    UNIT_ASSERT(hashVector == hashVectorExpected);
}

Y_UNIT_TEST(SomeText) {
    const TUtf16String text = u"  Какой-нибудь  текст (со скобочками) и ударе\u0301нием, и пере\u00ADносом, и буквой Ёё, и смайликом ☺, и иероглифом 呆 -- чтобы !!!ПРОВЕРИТЬ!!!, \n\r А) \r\n Б) \n В) \r Г) ... \"\' как он тут <{[123.45]}> нормализуется...    ";

    const THashVector hashVectorExpected = {10249, 21191, 2600, 50699, 7100, 1954, 18973, 1954, 13821, 1954, 37287, 51437, 1954, 38517, 1954, 58543, 26659, 5314, 5281, 59977, 61965, 63853, 45466, 29369, 57142, 12769, 13386, 57494, 10696,};

    const THashVector hashVector = DynamicListReplacerHashVector(text);

    UNIT_ASSERT(hashVector == hashVectorExpected);
}

Y_UNIT_TEST(SomeTextUtf8) {
    const TString text = "  Какой-нибудь  текст (со скобочками) и ударе\u0301нием, и пере\u00ADносом, и буквой Ёё, и смайликом ☺, и иероглифом 呆 -- чтобы !!!ПРОВЕРИТЬ!!!, \n\r А) \r\n Б) \n В) \r Г) ... \"\' как он тут <{[123.45]}> нормализуется...    ";

    const THashVector hashVectorExpected = {10249, 21191, 2600, 50699, 7100, 1954, 18973, 1954, 13821, 1954, 37287, 51437, 1954, 38517, 1954, 58543, 26659, 5314, 5281, 59977, 61965, 63853, 45466, 29369, 57142, 12769, 13386, 57494, 10696,};

    const THashVector hashVector = DynamicListReplacerHashVector(text);

    UNIT_ASSERT(hashVector == hashVectorExpected);
}

}  // DynamicListReplacerHashVector
