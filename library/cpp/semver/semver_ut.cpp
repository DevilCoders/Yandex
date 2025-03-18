#include "semver.h"
#include <library/cpp/testing/unittest/registar.h>

namespace {

TFullSemVer V(TStringBuf s) {
    return TFullSemVer::FromString(s).GetRef();
}

}

Y_UNIT_TEST_SUITE(TSemVerTest) {

Y_UNIT_TEST(ParseOk) {
    {
        TMaybe<TSemVer> sv = TSemVer::FromString("1.21.31");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Minor);
        UNIT_ASSERT_VALUES_EQUAL(31, sv->Patch);
        UNIT_ASSERT_VALUES_EQUAL(0, sv->Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.31", sv->ToString());
    }
    {
        TMaybe<TSemVer> sv = TSemVer::FromString("1.21");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Minor);
        UNIT_ASSERT_VALUES_EQUAL(0, sv->Patch);
        UNIT_ASSERT_VALUES_EQUAL(0, sv->Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.0", sv->ToString());
    }
    {
        TMaybe<TSemVer> sv = TSemVer::FromString("1");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Major);
        UNIT_ASSERT_VALUES_EQUAL(0, sv->Minor);
        UNIT_ASSERT_VALUES_EQUAL(0, sv->Patch);
        UNIT_ASSERT_VALUES_EQUAL(0, sv->Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.0.0", sv->ToString());
    }
    {
        TMaybe<TSemVer> sv = TSemVer::FromString("1.21.31.41");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Minor);
        UNIT_ASSERT_VALUES_EQUAL(31, sv->Patch);
        UNIT_ASSERT_VALUES_EQUAL(41, sv->Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.31.41", sv->ToString());
    }
    {
        TMaybe<TSemVer> sv = TSemVer::FromString("1.21.31.41.51");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Minor);
        UNIT_ASSERT_VALUES_EQUAL(31, sv->Patch);
        UNIT_ASSERT_VALUES_EQUAL(41, sv->Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.31.41", sv->ToString());
    }
}

Y_UNIT_TEST(FullParseOk) {
    {
        TMaybe<TFullSemVer> sv = TFullSemVer::FromString("1.21-dev");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Version.Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Version.Minor);
        UNIT_ASSERT_VALUES_EQUAL(0, sv->Version.Patch);
        UNIT_ASSERT_VALUES_EQUAL(0, sv->Version.Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.0-dev", sv->ToString());
    }
    {
        TMaybe<TFullSemVer> sv = TFullSemVer::FromString("1.21.31-dev");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Version.Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Version.Minor);
        UNIT_ASSERT_VALUES_EQUAL(31, sv->Version.Patch);
        UNIT_ASSERT_VALUES_EQUAL(0, sv->Version.Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.31-dev", sv->ToString());
    }
    {
        TMaybe<TFullSemVer> sv = TFullSemVer::FromString("1.21.31.41-dev");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Version.Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Version.Minor);
        UNIT_ASSERT_VALUES_EQUAL(31, sv->Version.Patch);
        UNIT_ASSERT_VALUES_EQUAL(41, sv->Version.Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.31.41-dev", sv->ToString());
    }
    {
        TMaybe<TFullSemVer> sv = TFullSemVer::FromString("1.21.31.41.51-dev");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Version.Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Version.Minor);
        UNIT_ASSERT_VALUES_EQUAL(31, sv->Version.Patch);
        UNIT_ASSERT_VALUES_EQUAL(41, sv->Version.Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.31.41-dev", sv->ToString());
    }
    {
        TMaybe<TFullSemVer> sv = TFullSemVer::FromString("1.21.31.41.51-dev.prod.100500");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Version.Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Version.Minor);
        UNIT_ASSERT_VALUES_EQUAL(31, sv->Version.Patch);
        UNIT_ASSERT_VALUES_EQUAL(41, sv->Version.Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.31.41-dev.prod.100500", sv->ToString());
    }
    {
        TMaybe<TFullSemVer> sv = TFullSemVer::FromString("1.21.31.41.51-dev.prod.100500+built.on.my.iphone");
        UNIT_ASSERT_VALUES_EQUAL(1, sv->Version.Major);
        UNIT_ASSERT_VALUES_EQUAL(21, sv->Version.Minor);
        UNIT_ASSERT_VALUES_EQUAL(31, sv->Version.Patch);
        UNIT_ASSERT_VALUES_EQUAL(41, sv->Version.Build);
        UNIT_ASSERT_STRINGS_EQUAL("1.21.31.41-dev.prod.100500", sv->ToString());
    }
}

Y_UNIT_TEST(ParseFail) {
    {
        TMaybe<TSemVer> sv = TSemVer::FromString("any string");
        UNIT_ASSERT_VALUES_EQUAL(true, sv.Empty());
    }
    {
        TMaybe<TSemVer> sv = TSemVer::FromString("1.2.a");
        UNIT_ASSERT_VALUES_EQUAL(true, sv.Empty());
    }
}

Y_UNIT_TEST(SemVerCompare) {
    TSemVer v{10, 20, 30};

    {
        UNIT_ASSERT_VALUES_EQUAL(v, v);
        UNIT_ASSERT_EQUAL(true,  v == v);
        UNIT_ASSERT_EQUAL(false, v >  v);
        UNIT_ASSERT_EQUAL(false, v <  v);
        UNIT_ASSERT_EQUAL(true,  v <= v);
        UNIT_ASSERT_EQUAL(true,  v >= v);
    }

    {
        TSemVer v1{static_cast<ui8>(v.Major + 1), static_cast<ui8>(v.Minor - 1), static_cast<ui8>(v.Patch - 1)};
        UNIT_ASSERT_EQUAL(true,  v1 >  v);
        UNIT_ASSERT_EQUAL(true,  v1 >= v);
        UNIT_ASSERT_EQUAL(true,  v1 != v);
        UNIT_ASSERT_EQUAL(false, v1 == v);
        UNIT_ASSERT_EQUAL(false, v1 <= v);
        UNIT_ASSERT_EQUAL(false, v1  < v);
    }

    {
        TSemVer v1{v.Major, static_cast<ui8>(v.Minor + 1), static_cast<ui8>(v.Patch - 1)};
        UNIT_ASSERT_EQUAL(true,  v1 >  v);
        UNIT_ASSERT_EQUAL(true,  v1 >= v);
        UNIT_ASSERT_EQUAL(true,  v1 != v);
        UNIT_ASSERT_EQUAL(false, v1 == v);
        UNIT_ASSERT_EQUAL(false, v1 <= v);
        UNIT_ASSERT_EQUAL(false, v1  < v);
    }

    {
        TSemVer v1{v.Major, v.Minor, static_cast<ui8>(v.Patch + 1)};
        UNIT_ASSERT_EQUAL(true,  v1 >  v);
        UNIT_ASSERT_EQUAL(true,  v1 >= v);
        UNIT_ASSERT_EQUAL(true,  v1 != v);
        UNIT_ASSERT_EQUAL(false, v1 == v);
        UNIT_ASSERT_EQUAL(false, v1 <= v);
        UNIT_ASSERT_EQUAL(false, v1  < v);
    }
}

Y_UNIT_TEST(FullSemVerCompareSameItem) {
    const TVector<TFullSemVer> items = {
        V("1.2.3-alpha"),
        V("1.2.3"),
        V("1.2"),
        V("1"),
        V("1-alpha"),
    };

    for (const auto& item : items) {
        UNIT_ASSERT_VALUES_EQUAL(item, item);
        UNIT_ASSERT_EQUAL(true,  item == item);
        UNIT_ASSERT_EQUAL(false, item >  item);
        UNIT_ASSERT_EQUAL(false, item <  item);
        UNIT_ASSERT_EQUAL(true,  item <= item);
        UNIT_ASSERT_EQUAL(true,  item >= item);
    }
}

Y_UNIT_TEST(FullSemVerComparePreReleasePrecedence) {
    const TVector<TFullSemVer> items = {
        V("1.0.0-12+100"),
        V("1.0.0-alpha"),
        V("1.0.0-alpha.1"),
        V("1.0.0-alpha.beta"),
        V("1.0.0-beta"),
        V("1.0.0-beta.2"),
        V("1.0.0-beta.11"),
        V("1.0.0-rc.1"),
        V("1.0.0"),
    };
    // 1.0.0-12+100 < 1.0.0-alpha < 1.0.0-alpha.1 < 1.0.0-alpha.beta < 1.0.0-beta < 1.0.0-beta.2 < 1.0.0-beta.11 < 1.0.0-rc.1 < 1.0.0.
    const TFullSemVer* prevItem = nullptr;
    for (const auto& item : items) {
        if (prevItem) {
            UNIT_ASSERT_C(*prevItem < item, TStringBuilder() << *prevItem << " < " << item);
            UNIT_ASSERT_C(*prevItem <= item, TStringBuilder() << *prevItem << " <= " << item);
            UNIT_ASSERT_C(item > *prevItem, TStringBuilder() << item << " > " << *prevItem);
            UNIT_ASSERT_C(item >= *prevItem, TStringBuilder() << item << " >= " << *prevItem);
        }

        prevItem = &item;
    }
}

Y_UNIT_TEST(FullSemVerCompareLessPreRelease) {
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha.1")    < V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      < V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-beta")       < V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      < V("1.2.3-beta"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      < V("1.2.3-alpha.1"));

    // Numeric identifiers always have lower precedence than non-numeric identifiers (in prerelease).
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-0.12")       < V("1.2.3-alpha.1"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-0.12")       < V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      < V("1.2.3-100"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha.1")    < V("1.2.3-alpha.beta"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha.beta") < V("1.2.3-alpha.1"));

    // When major, minor, and patch are equal, a pre-release version has lower precedence than a normal version.
    UNIT_ASSERT_EQUAL(false, V("1.2.3")            < V("1.2.3-alpha.1"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      < V("1.2.3"));
}

Y_UNIT_TEST(FullSemVerCompareGreaterPreRelease) {
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha.1")    > V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      > V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-beta")       > V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      > V("1.2.3-beta"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      > V("1.2.3-alpha.1"));

    // Numeric identifiers always have lower precedence than non-numeric identifiers (in prerelease).
    UNIT_ASSERT_EQUAL(false, V("1.2.3-0.12")       > V("1.2.3-alpha.1"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-0.12")       > V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      > V("1.2.3-100"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha.1")    > V("1.2.3-alpha.beta"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha.beta") > V("1.2.3-alpha.1"));

    // When major, minor, and patch are equal, a pre-release version has lower precedence than a normal version.
    UNIT_ASSERT_EQUAL(true,  V("1.2.3")            > V("1.2.3-alpha.1"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      > V("1.2.3"));
}

Y_UNIT_TEST(FullSemVerCompareGreaterOrEqualPreRelease) {
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha.1")    >= V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      >= V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-beta")       >= V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-beta.1")     >= V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-beta.1")     >= V("1.2.3-alpha.100"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-beta.1")     >= V("1.2.3-alpha.beta"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-beta.zetta") >= V("1.2.3-beta.1"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      >= V("1.2.3-beta"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      >= V("1.2.3-alpha.1"));

    // Numeric identifiers always have lower precedence than non-numeric identifiers (in prerelease).
    UNIT_ASSERT_EQUAL(false, V("1.2.3-0.12")       >= V("1.2.3-alpha.1"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-0.12")       >= V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      >= V("1.2.3-100"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-101")        >= V("1.2.3-100"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha.1")    >= V("1.2.3-alpha.beta"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha.beta") >= V("1.2.3-alpha.1"));

    // When major, minor, and patch are equal, a pre-release version has lower precedence than a normal version.
    UNIT_ASSERT_EQUAL(true,  V("1.2.3")            >= V("1.2.3-alpha.1"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      >= V("1.2.3"));
}

Y_UNIT_TEST(FullSemVerCompareLessOrEqualPreRelease) {
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha.1")    <= V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      <= V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-beta")       <= V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      <= V("1.2.3-beta"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      <= V("1.2.3-alpha.1"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      <= V("1.2.3-alpha"));

    // Numeric identifiers always have lower precedence than non-numeric identifiers (in prerelease).
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-0.12")       <= V("1.2.3-alpha.1"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-0.12")       <= V("1.2.3-alpha"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha")      <= V("1.2.3-100"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha.1")    <= V("1.2.3-alpha.beta"));
    UNIT_ASSERT_EQUAL(false, V("1.2.3-alpha.beta") <= V("1.2.3-alpha.1"));

    // When major, minor, and patch are equal, a pre-release version has lower precedence than a normal version.
    UNIT_ASSERT_EQUAL(false, V("1.2.3")            <= V("1.2.3-alpha.1"));
    UNIT_ASSERT_EQUAL(true,  V("1.2.3-alpha")      <= V("1.2.3"));
}

Y_UNIT_TEST(FullCompare) {
    UNIT_ASSERT(V("1.2.4") >  V("1.2.3"));
    UNIT_ASSERT(V("1.2.4") >= V("1.2.3"));
    UNIT_ASSERT(V("1.2.2") <  V("1.2.3"));
    UNIT_ASSERT(V("1.2.2") <= V("1.2.3"));

    UNIT_ASSERT(V("1.2.4").Version >  TSemVer(1, 2, 3));
    UNIT_ASSERT(V("1.2.4").Version >= TSemVer(1, 2, 3));
    UNIT_ASSERT(V("1.2.2").Version <  TSemVer(1, 2, 3));
    UNIT_ASSERT(V("1.2.2").Version <= TSemVer(1, 2, 3));

    UNIT_ASSERT(V("1.2.3-alpha").Version == TSemVer(1, 2, 3));

    UNIT_ASSERT(V("1.2.3-10") <= V("1.2.3-10+100500"));
    UNIT_ASSERT(V("1.2.3-10") == V("1.2.3-10+100500"));
}

}
