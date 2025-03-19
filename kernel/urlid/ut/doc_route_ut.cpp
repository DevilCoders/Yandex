#include <kernel/urlid/doc_route.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TDocRoute) {
    Y_UNIT_TEST(DefaultConstructor) {
        TDocRoute dr;
        // otherwise both clang 3.8 and GCC 6.1 emit
        // error: undefined reference to 'TDocRoute::NoSource'
        const auto ns = dr.NoSource;
        const auto&& sNoSrc = ToString(ns);
        for (ui16 i = 0; i < dr.MaxLength; ++i)
            UNIT_ASSERT_EQUAL_C(dr.Source(i), dr.NoSource,
                                "\n\tdr.Source(" + ToString (i) + ") = "
                                + ToString (dr.Source(i)) + ", expected: dr.NoSource, which is " + sNoSrc);
        UNIT_ASSERT_EQUAL(dr.Length(), 0);
        UNIT_ASSERT_EQUAL(dr.Next(), dr.NoSource);
        for (int i = 0; i < dr.TotalFlags; ++i)
            UNIT_ASSERT_C(dr.GetFlag(i), "The flag " + ToString(i) + " is expected to be set");
    }

    Y_UNIT_TEST(IntConstructor) {
        TDocRoute sample;
        for (ui16 i = 0; i < sample.MaxLength; ++i)
            sample.SetSource(i, i + 1);
        TDocRoute dr (sample.Raw());
        for (ui16 i = 0; i < dr.MaxLength; ++i)
            UNIT_ASSERT_EQUAL_C(dr.Source(i), sample.Source(i),
                                "\n\tdr.Source(" + ToString (i) + ") = "
                                + ToString (dr.Source(i)) + ", expected: sample.Source(" + ToString(i)
                                + "), which is " + ToString(sample.Source(i)));
        for (int i = 0; i < dr.TotalFlags; ++i)
            UNIT_ASSERT_C(dr.GetFlag(i), "The flag " + ToString(i) + " is expected to be set");
    }

    Y_UNIT_TEST(ListConstructor) {
        TDocRoute dr({1, 2, 3});
        UNIT_ASSERT_VALUES_EQUAL(dr.Length(), 3);
        UNIT_ASSERT_VALUES_EQUAL(dr.Next(), 3);
        UNIT_ASSERT_VALUES_EQUAL(dr.Source(0), 1);
        UNIT_ASSERT_VALUES_EQUAL(dr.Source(1), 2);
        UNIT_ASSERT_VALUES_EQUAL(dr.Source(2), 3);
        for (int i = 0; i < dr.TotalFlags; ++i)
            UNIT_ASSERT_C(dr.GetFlag(i), "The flag " + ToString(i) + " is expected to be set");
    }

    Y_UNIT_TEST(SetSource) {
        TDocRoute dr;
        for (ui16 i = 0; i < dr.MaxLength; ++i)
            dr.SetSource(i, i + 1);
        for (ui16 i = 0; i < dr.MaxLength; ++i)
            UNIT_ASSERT_EQUAL_C(dr.Source(i), i + 1,
                                "\n\tdr.Source(" + ToString (i) + ") = "
                                + ToString (dr.Source(i)) + ", expected: " + ToString(i + 1));
        for (int i = 0; i < dr.TotalFlags; ++i)
            UNIT_ASSERT_C(dr.GetFlag(i), "The flag " + ToString(i) + " is expected to be set");
    }

    Y_UNIT_TEST(SetFlag) {
        TDocRoute dr;
        for (int i = 0; i < dr.TotalFlags; ++i) {
            UNIT_ASSERT_C(dr.GetFlag(i), "The flag " + ToString(i) + " is expected to be set");
            dr.SetFlag(i, false);
            UNIT_ASSERT_C(!dr.GetFlag(i), "The flag " + ToString(i) + " is expected to be cleared");
            UNIT_ASSERT_VALUES_EQUAL(dr.Length(), 0);
            // Length will not notice if we spoil only the highest source number
            for (ui16 i = 0; i < dr.MaxLength; ++i) {
                const auto noSource = dr.NoSource; //< specially for ToString (avoiding linker error)
                UNIT_ASSERT_EQUAL_C(dr.Source(i), noSource,
                                    "\n\tdr.Source(" + ToString (i) + ") = "
                                    + ToString (dr.Source(i)) + ", expected: " + ToString(noSource));
            }
        }
    }

    Y_UNIT_TEST(ClearSource) {
        TDocRoute dr;
        for (ui16 i = 0; i < dr.MaxLength; ++i)
            dr.SetSource(i, i + 1);

        for (ui16 i = 0; i < dr.MaxLength; ++i) {
            dr.ClearSource(i);
            for (ui16 j = i + 1; j < dr.MaxLength; ++j)
                // check that nothing else is spoiled
                UNIT_ASSERT_EQUAL_C(dr.Source(j), j + 1,
                                    "\n\tdr.Source(" + ToString (j) + ") = "
                                    + ToString (dr.Source(j)) + ", expected: " + ToString(j + 1));
        }
        UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 0u);
        for (int i = 0; i < dr.TotalFlags; ++i)
            UNIT_ASSERT_C(dr.GetFlag(i), "The flag " + ToString(i) + " is expected to be set");
    }

    Y_UNIT_TEST(Length) {
        TDocRoute dr;
        // static_cast is here for better error diagnostics (otherwise Length is treated as char)
        UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 0u);
        dr.PushFront(1);
        UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 1u);
        for (int i = 0; i < dr.TotalFlags; ++i)
            UNIT_ASSERT_C(dr.GetFlag(i), "The flag " + ToString(i) + " is expected to be set");
    }

    Y_UNIT_TEST(Clear) {
        TDocRoute dr;
        for (ui16 i = 0; i < dr.MaxLength; ++i)
            dr.SetSource(i, i + 1);

        for (int i = 0; i < dr.TotalFlags; ++i) {
            dr.SetFlag(i, false);
            UNIT_ASSERT_C(!dr.GetFlag(i), "The flag " + ToString(i) + " is expected to be cleared");
        }

        const auto maxlen = dr.MaxLength;
        UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), maxlen);
        dr.Clear();
        UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 0u);
        const auto noSource = dr.NoSource;
        for (ui16 i = 0; i < dr.MaxLength; ++i)
                UNIT_ASSERT_EQUAL_C(dr.Source(i), dr.NoSource,
                                    "\n\tdr.Source(" + ToString (i) + ") = "
                                    + ToString (dr.Source(i)) + ", expected: NoSource, which is " + ToString(noSource));
        for (int i = 0; i < dr.TotalFlags; ++i)
            UNIT_ASSERT_C(!dr.GetFlag(i), "The flag " + ToString(i) + " is modified by Clear()");
    }

    Y_UNIT_TEST(ToDocId) {
        {
            TDocRoute dr;
            UNIT_ASSERT_VALUES_EQUAL(dr.ToDocId(), "");
        }
        {
            TDocRoute dr;
            static_assert (4 < dr.MaxLength, "Fix this test");
            for (ui16 i = 0; i < 4; ++i)
                dr.SetSource(i, i + 1);
            TString strRoute = TString("4") + dr.Separator + "3" + dr.Separator + "2" + dr.Separator + "1";
            UNIT_ASSERT_VALUES_EQUAL(dr.ToDocId(), strRoute);
        }
    }

    Y_UNIT_TEST(PushPopFront) {
        TDocRoute dr;
        dr.PopFront();
        UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 0);
        dr.PushFront(dr.MaxSourceNumber - 1);
        static_assert(dr.NoSource != dr.MaxSourceNumber - 1, "Fix this test");
        UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 1);
        UNIT_ASSERT_VALUES_EQUAL(dr.Source(0), dr.MaxSourceNumber - 1);
        dr.PopFront();
        UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 0);

        for (ui16 i = 0; i < dr.MaxLength; ++i) {
            dr.PushFront(i + 1);
            UNIT_ASSERT_VALUES_EQUAL (dr.Next(), i + 1);
        }
        for (ui16 i = 0; i < dr.MaxLength; ++i)
            UNIT_ASSERT_EQUAL_C(dr.Source(i), i + 1,
                                "\n\tdr.Source(" + ToString (i) + ") = "
                                + ToString (dr.Source(i)) + ", expected: " + ToString(i + 1));

        const auto ml = dr.MaxLength; //< avoid 'undefined reference to dr.MaxLength'
        UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), ml);

        for (ui16 i = dr.MaxLength; i --> 0;) {
            // check that PopFront does not spoil values left in the route:
            UNIT_ASSERT_EQUAL_C(dr.Source(i), i + 1,
                                "\n\tdr.Source(" + ToString (i) + ") = "
                                + ToString (dr.Source(i)) + ", expected: " + ToString(i + 1));
            dr.PopFront();
            UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), i);
        }
    }

    Y_UNIT_TEST(PushBack) {
        TDocRoute dr;
        for (ui16 i = 0; i < dr.MaxLength; ++i) {
            dr.PushBack(i + 1);
            UNIT_ASSERT_VALUES_EQUAL(dr.Length(), i + 1);
            for (ui16 j = 0; j <= i; ++j) {
                UNIT_ASSERT_VALUES_EQUAL(dr.Source(j), i - j + 1);
            }
        }
    }

    Y_UNIT_TEST(FromDocid){
        {
            TDocRoute dr = TDocRoute::FromDocId("");
            UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 0);
            UNIT_ASSERT_VALUES_EQUAL(dr.ToDocId(), "");
        }
        {
            TString strDocRoute = TString("1") + TDocRoute::Separator + "2" + TDocRoute::Separator + "3";
            TString docid = strDocRoute + TDocRoute::Separator + "ZHash";
            TDocRoute dr = TDocRoute::FromDocId(docid);
            UNIT_ASSERT_VALUES_EQUAL(dr.ToDocId(), strDocRoute);
            UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 3);
            UNIT_ASSERT_VALUES_EQUAL(dr.Next(), 1);
            UNIT_ASSERT_VALUES_EQUAL(dr.Source(0), 3);
            UNIT_ASSERT_VALUES_EQUAL(dr.Source(1), 2);
            UNIT_ASSERT_VALUES_EQUAL(dr.Source(2), 1);
        }
        {
            TString strDocRoute = TString("1") + TDocRoute::Separator + "2" + TDocRoute::Separator + "3";
            TString docid = strDocRoute + TDocRoute::Separator + "Hash";
            TStringStream ss;
            ss << docid;
            TDocRoute dr;
            ss >> dr;
            UNIT_ASSERT_VALUES_EQUAL(dr.ToDocId(), strDocRoute);
            UNIT_ASSERT_VALUES_EQUAL(static_cast<unsigned>(dr.Length()), 3);
            UNIT_ASSERT_VALUES_EQUAL(dr.Next(), 1);
            UNIT_ASSERT_VALUES_EQUAL(dr.Source(0), 3);
            UNIT_ASSERT_VALUES_EQUAL(dr.Source(1), 2);
            UNIT_ASSERT_VALUES_EQUAL(dr.Source(2), 1);
        }
        {
            TString strDocRoute;
            for (ui8 l = 0; l < TDocRoute::MaxLength; ++l)
                strDocRoute += ToString (l) + TDocRoute::Separator;
            TDocRoute dr = TDocRoute::FromDocId(strDocRoute  + "ZHash");
            UNIT_ASSERT_VALUES_EQUAL(dr.ToDocId()+ TDocRoute::Separator, strDocRoute);
        }
        {
            TString strDocRoute;
            for (ui8 l = 0; l < TDocRoute::MaxLength + 1; ++l)
                strDocRoute += ToString (l) + TDocRoute::Separator;
            strDocRoute += "ZHello";
            UNIT_ASSERT_EXCEPTION(TDocRoute::FromDocId(strDocRoute), yexception);
        }
    }
}
