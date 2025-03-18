#include "tools.h"

#include <library/cpp/on_disk/mms/string.h>

#include <library/cpp/testing/unittest/registar.h>

template <class S1, class S2>
void CheckEqualStrings(const S1& lhs, const S2& rhs) {
    UNIT_ASSERT_EQUAL(lhs.size(), rhs.size());
    UNIT_ASSERT_EQUAL(lhs.length(), rhs.length());
    UNIT_ASSERT_EQUAL(lhs.size(), rhs.size());

    for (size_t i = 0; i < lhs.length(); ++i) {
        UNIT_ASSERT_EQUAL(lhs[i], rhs[i]);
        UNIT_ASSERT_EQUAL(*(lhs.data() + i), *(rhs.data() + i));
        UNIT_ASSERT_EQUAL(lhs.at(i), rhs.at(i));
    }

    UNIT_ASSERT_EQUAL(lhs, rhs);
    UNIT_ASSERT(lhs == rhs);
    UNIT_ASSERT(rhs == lhs);
    UNIT_ASSERT(!(lhs > rhs));
    UNIT_ASSERT(!(lhs < rhs));
    UNIT_ASSERT((lhs >= rhs));
    UNIT_ASSERT((lhs <= rhs));
    UNIT_ASSERT(!(lhs != rhs));

    UNIT_ASSERT_EQUAL(lhs, lhs);
    UNIT_ASSERT(lhs == lhs);
    UNIT_ASSERT(!(lhs > lhs));
    UNIT_ASSERT(!(lhs < lhs));
    UNIT_ASSERT((lhs >= lhs));
    UNIT_ASSERT((lhs <= lhs));
    UNIT_ASSERT(!(lhs != lhs));

    UNIT_ASSERT_VALUES_EQUAL(THash<S1>()(lhs), THash<S2>()(rhs));
}

void CheckEqualStringsAll(const TString& s) {
    TMmsObjects objs;

    TStringBuf sb(s);
    NMms::TStringType<NMms::TMmapped> mm = objs.MakeMmappedString(s);
    NMms::TStringType<NMms::TStandalone> sa(s);

    CheckEqualStrings(s, s);
    CheckEqualStrings(s, mm);
    CheckEqualStrings(s, sa);
    CheckEqualStrings(s, sb);

    CheckEqualStrings(mm, s);
    CheckEqualStrings(mm, sa);
    CheckEqualStrings(mm, mm);
    CheckEqualStrings(mm, sb);

    CheckEqualStrings(sa, s);
    CheckEqualStrings(sa, mm);
    CheckEqualStrings(sa, sa);
    CheckEqualStrings(sa, sb);

    CheckEqualStrings(sb, s);
    CheckEqualStrings(sb, mm);
    CheckEqualStrings(sb, sa);
    CheckEqualStrings(sb, sb);
}

template <class S1, class S2>
void CheckLessStrings(const S1& lhs, const S2& rhs) {
    UNIT_ASSERT(!(lhs == rhs));
    UNIT_ASSERT(!(rhs == lhs));
    UNIT_ASSERT(lhs != rhs);
    UNIT_ASSERT(rhs != lhs);

    UNIT_ASSERT(lhs < rhs);
    UNIT_ASSERT(lhs <= rhs);

    UNIT_ASSERT(!(lhs > rhs));
    UNIT_ASSERT(!(lhs >= rhs));

    UNIT_ASSERT(!(rhs < lhs));
    UNIT_ASSERT(!(rhs <= lhs));

    UNIT_ASSERT(rhs > lhs);
    UNIT_ASSERT(rhs >= lhs);

    // It is not guaranteed that hashes of different strings are different,
    // but it is probably true. Comment out this line in case of problems.
    UNIT_ASSERT_VALUES_UNEQUAL(THash<S1>()(lhs), THash<S2>()(rhs));
}

void CheckLessStringsAll(const TString& s1, const TString& s2) {
    TMmsObjects objs;

    TStringBuf sb1(s1);
    NMms::TStringType<NMms::TMmapped> mm1 = objs.MakeMmappedString(s1);
    NMms::TStringType<NMms::TStandalone> sa1(s1);

    TStringBuf sb2(s2);
    NMms::TStringType<NMms::TMmapped> mm2 = objs.MakeMmappedString(s2);
    NMms::TStringType<NMms::TStandalone> sa2(s2);

    CheckLessStrings(s1, s2);
    CheckLessStrings(s1, mm2);
    CheckLessStrings(s1, sa2);
    CheckLessStrings(s1, sb2);

    CheckLessStrings(mm1, s2);
    CheckLessStrings(mm1, mm2);
    CheckLessStrings(mm1, sa2);
    CheckLessStrings(mm1, sb2);

    CheckLessStrings(sa1, s2);
    CheckLessStrings(sa1, mm2);
    CheckLessStrings(sa1, sa2);
    CheckLessStrings(sa1, sb2);

    CheckLessStrings(sb1, s2);
    CheckLessStrings(sb1, mm2);
    CheckLessStrings(sb1, sa2);
    CheckLessStrings(sb1, sb2);
}

Y_UNIT_TEST_SUITE(TMmsStringComparatorTest) {
    Y_UNIT_TEST(EqualCmpTest) {
        CheckEqualStringsAll("");
        CheckEqualStringsAll("123");
        CheckEqualStringsAll("dakljsklekjl");
        CheckEqualStringsAll("abacaba");
        CheckEqualStringsAll(TString("3290"
                                     "\0"
                                     "37289",
                                     10));
    }

    Y_UNIT_TEST(LessCmpTest) {
        CheckLessStringsAll("", "a");
        CheckLessStringsAll("123", "1234");
        CheckLessStringsAll(TString("123"
                                    "\0",
                                    4),
                            "1234");
        CheckLessStringsAll("123", TString("123"
                                           "\0",
                                           4));
        CheckLessStringsAll(TString("123"
                                    "\0"
                                    "2",
                                    5),
                            TString("123"
                                    "\0"
                                    "3",
                                    5));
        CheckLessStringsAll(TString("123"
                                    "\0"
                                    "2"
                                    "\0",
                                    6),
                            TString("123"
                                    "\0"
                                    "2"
                                    "\0"
                                    "2",
                                    7));
        CheckLessStringsAll("123432", "444");
        CheckLessStringsAll("abacab", "abacaba");
    }
}
