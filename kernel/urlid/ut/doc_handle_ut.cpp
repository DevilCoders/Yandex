#include <kernel/urlid/doc_handle.h>
#include <kernel/urlid/urlid.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/vector.h>
#include <util/ysaveload.h>

namespace {
    // Warning: this value is an implementation detail from dochandle.cpp, not exposed anywhere, but necessary to validate de/serialization
    // If this value is changed, just change it also here to fix tests.
    constexpr char IndexDelimiter = ',';

    static_assert(sizeof(TDocRoute::Separator) == 1, "Update the line below");
    const auto DocRouteSeparator = TDocRoute::Separator; //< runtime reference for TStringBuf()
    const TStringBuf RouteSeparator(&DocRouteSeparator, 1); //< an argument suitable for JoinStrings
}

Y_UNIT_TEST_SUITE(TDocHandle) {
    Y_UNIT_TEST(Constructor) {
        TDocHandle di;
        const auto ih = di.InvalidHash;
        UNIT_ASSERT_VALUES_EQUAL(std::get<TDocHandle::THash>(di.DocHash), ih);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 0);
        UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, 0);
        UNIT_ASSERT_VALUES_EQUAL(static_cast<bool>(di), false);
        UNIT_ASSERT(!di.IsUnique());
    }

    Y_UNIT_TEST(FromEmptyString) {
        TDocHandle di("");
        const auto ih = di.InvalidHash;
        UNIT_ASSERT_VALUES_EQUAL(std::get<TDocHandle::THash>(di.DocHash), ih);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 0);
        UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, 0);
        UNIT_ASSERT(!di.IsUnique());
    }

    Y_UNIT_TEST(FromNonNullTermString) {
        const char data[] = {'h', 'e', 'l', 'l', 'o'};
        TStringBuf buf(data, sizeof(data));
        TDocHandle dh;
        UNIT_ASSERT_NO_EXCEPTION(dh.FromString(buf));
        UNIT_ASSERT_VALUES_EQUAL(dh.ToString(), "hello");
    }

    Y_UNIT_TEST(FromHash) {
        const TDocHandle::THash hashVal = 1234567800000000;
        TString strHash = "1234567800000000";
        TDocHandle di(strHash);
        UNIT_ASSERT_VALUES_EQUAL(di.IntHash(), hashVal);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 0);
        UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, 0);
        UNIT_ASSERT(!di.IsUnique());
        UNIT_ASSERT(!di.IsString());
    }

    Y_UNIT_TEST(FromHashWithIndex) {
        const NUrlId::TUrlId hashVal = 1234567800000000;
        TString strHash = "1234567800000000";
        strHash += IndexDelimiter;
        strHash += "123";
        TDocHandle di(strHash.data());
        UNIT_ASSERT_VALUES_EQUAL(di.IntHash(), hashVal);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 0);
        UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, 123);
        UNIT_ASSERT(!di.IsUnique());
        UNIT_ASSERT(!di.IsString());
    }

    Y_UNIT_TEST(FromHandle) {
        const NUrlId::TUrlId hashVal = 1234567800000000;
        const auto s = TDocRoute::Separator;
        TString strHash = TString("1") + s + "2" + s + "3" + s + "1234567800000000" + IndexDelimiter + "123";
        TDocHandle di(strHash.data());
        TDocHandle::THash parsed;
        UNIT_ASSERT_VALUES_EQUAL(std::get<TDocHandle::THash>(di.DocHash), hashVal);
        UNIT_ASSERT(di.TryIntHash(parsed));
        UNIT_ASSERT_VALUES_EQUAL(parsed, hashVal);
        UNIT_ASSERT_VALUES_EQUAL(di.IntHash(), hashVal);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Source(0), 3);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Source(1), 2);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Source(2), 1);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Next(), 1);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 3);
        UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, 123);
        UNIT_ASSERT(!di.IsUnique());

        di.FromString("0000000012345678");
        // Check that FromString clears other fields
        UNIT_ASSERT_VALUES_EQUAL(di.IntHash(), 12345678);
        UNIT_ASSERT(di.TryIntHash(parsed));
        UNIT_ASSERT_VALUES_EQUAL(parsed, 12345678);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 0);
        UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, UndefIndGenValue);
    }

    Y_UNIT_TEST(FromZeros) {
        TDocHandle di;
        TDocHandle::THash parsed;

        di.FromString("0");
        UNIT_ASSERT(!di.IsString());
        UNIT_ASSERT(di.TryIntHash(parsed));
        UNIT_ASSERT_VALUES_EQUAL(parsed, 0);
        UNIT_ASSERT_VALUES_EQUAL(di.IntHash(), 0);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 0);
        UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, UndefIndGenValue);

        di.FromString("0-0");
        UNIT_ASSERT(!di.IsString());
        UNIT_ASSERT(di.TryIntHash(parsed));
        UNIT_ASSERT_VALUES_EQUAL(parsed, 0);
        UNIT_ASSERT_VALUES_EQUAL(di.IntHash(), 0);
        UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 1);
        UNIT_ASSERT_VALUES_EQUAL(di.ClientNum(), 0);
        UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, UndefIndGenValue);
    }

    Y_UNIT_TEST(FromZHandleFlags) {
        TDocHandle dh("Z12345678DEADBEEF");
        dh.FromString("Hello");
        // Check that FromString clears flags
        UNIT_ASSERT_VALUES_EQUAL(dh.ToString(), "Hello");
        UNIT_ASSERT(!dh.IsUnique());
        UNIT_ASSERT(dh.IsString());
    }

    Y_UNIT_TEST(FromZHandle) {
        const NUrlId::TUrlId hashVal = NUrlId::Str2Hash(TStringBuf ("12345678DEADBEEF"));
        const auto s = TDocRoute::Separator;
        TString strHash = "Z12345678DEADBEEF";
        TString strHandle = TString("1") + s + "2" + s + "3" + s + strHash + IndexDelimiter + "123";
        {
            TDocHandle di(strHandle);
            UNIT_ASSERT(di.IsUnique());
            UNIT_ASSERT_VALUES_EQUAL(std::get<TDocHandle::THash>(di.DocHash), hashVal);
            TDocHandle::THash parsed = 0;
            UNIT_ASSERT(di.TryIntHash(parsed));
            UNIT_ASSERT_VALUES_EQUAL(parsed, hashVal);
            UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Source(0), 3);
            UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Source(1), 2);
            UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Source(2), 1);
            UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Next(), 1);
            UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 3);
            UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, 123);
        }
        {
            TDocHandle di(strHash);
            UNIT_ASSERT(di.IsUnique());
            UNIT_ASSERT_VALUES_EQUAL(std::get<TDocHandle::THash>(di.DocHash), hashVal);
            UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 0);
            UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, UndefIndGenValue);
            TDocHandle::THash parsed = 0;
            UNIT_ASSERT(di.TryIntHash(parsed));
            UNIT_ASSERT_VALUES_EQUAL(parsed, hashVal);
        }
        {
            TDocHandle di("ZABCD");
            const TDocHandle::THash hashVal = NUrlId::Str2Hash(TString("ABCD"));
            UNIT_ASSERT(di.IsUnique());
            UNIT_ASSERT(di.IsString());
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), "ZABCD");
            UNIT_ASSERT_VALUES_EQUAL(di.IntHash(), hashVal);
            TDocHandle::THash parsed = 0;
            UNIT_ASSERT(di.TryIntHash(parsed));
            UNIT_ASSERT_VALUES_EQUAL(parsed, hashVal);
            UNIT_ASSERT_VALUES_EQUAL(di.DocRoute.Length(), 0);
            UNIT_ASSERT_VALUES_EQUAL(di.IndexGeneration, UndefIndGenValue);
        }
    }

    Y_UNIT_TEST(ToStringEmpty) {
        TDocHandle di;
        UNIT_ASSERT_VALUES_EQUAL(di.ToString(), "");
    }

    Y_UNIT_TEST(ToString) {
        const auto s = TDocRoute::Separator;
        TString strHash = "12345678DEADBEEF";
        TString  strHashWithRoute = TString("1") + s + "2" + s + "3" + s       + strHash;
        TString strZHashWithRoute = TString("1") + s + "2" + s + "3" + s + "Z" + strHash;
        TString strHashWithIdx = strHash + IndexDelimiter + "123";
        {
            TDocHandle di(strHashWithIdx);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.AppendIndGeneration), strHashWithIdx);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), strHash);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), ToString(di));
        }
        {
            TDocHandle di(strHashWithRoute);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintAll  ), strHashWithRoute);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintRoute), strHashWithRoute);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), ToString(di));
        }
        {
            TDocHandle di(strHash);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintAll  ), strHash);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintRoute), strHash);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), ToString(di));
        }
        {
            TDocHandle di(strZHashWithRoute);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintAll  ), strZHashWithRoute);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintRoute), strZHashWithRoute);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), ToString(di));
        }
        {
            TDocHandle di("Z" + strHash);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintAll), "Z" + strHash);
        }
        {
            TDocHandle di("111");
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), "111");
            UNIT_ASSERT_VALUES_EQUAL(ToString(di), "111");
        }
        {
            TDocHandle di("0");
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), "0");
        }
        {
            TDocHandle di("Z111");
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), "Z111");
        }
        {
            TDocHandle di("Z0");
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), "Z0");
        }
        {
            // wow, noapacheupper replies with such docids. Колдунщики мрут без этого.
            TDocHandle di(TString("52") + s);
            UNIT_ASSERT_VALUES_EQUAL(di.ToString(), TString("52") + s);
        }
    }

    Y_UNIT_TEST(NonFormatStrings){
        // Support for string dochashes is considered as a feature for legacy systems and may be removed once not needed anymore.
        const auto check = [](const TString& s){
            const TDocHandle dh(s);
            UNIT_ASSERT_VALUES_EQUAL(dh.ToString(), s);
            TDocHandle::THash result;
            UNIT_ASSERT(!dh.TryIntHash(result));
            UNIT_ASSERT_EXCEPTION(dh.IntHash(), yexception);
        };
        const TString s = TString() + TDocRoute::Separator;
        const TString is = TString() + IndexDelimiter;
        check("111A");
        check("A111A");
        check("ZAZAZAza");
        check("Hell0World");
        check("Hello, world!"); //< catching non-format index generation
        check("Z13412341234123412341234");
        check("1234567890ABCDEF"); //< non z-docid in hex
        check("1" + s + "2" + s + "3" + is + "4" + is + "5" + is + "6"); //< several index generations
        check("1" + s + "2" + is + "3" + s + "ZABCD"); //< route-like index generation is acceptable

        UNIT_ASSERT_EXCEPTION(TDocHandle("Bad" + s + "Route"), yexception);
        // Too long route
        UNIT_ASSERT_EXCEPTION(TDocHandle("1" + s + "2" + s + "3" + s + "4" + s + "5" + s + "6" + s + "7" + s + "8" + s + "Z1234567890ABCDEF"), yexception);
        UNIT_ASSERT_EXCEPTION(TDocHandle(s + s + "hello"), yexception);
        UNIT_ASSERT_EXCEPTION(TDocHandle(s + "hello"), yexception);
        UNIT_ASSERT_EXCEPTION(TDocHandle(s), yexception);
        // Non-format index generation
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle(is + "1234"));
        UNIT_ASSERT_EXCEPTION(TDocHandle("1234" + is), yexception);
        UNIT_ASSERT_EXCEPTION(TDocHandle(is + "1234" + is), yexception);
        // Testing index generation == UndefIndGenValue (falling back to string storage for correct serialization later)
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle(JoinStrings({"0", "0", "0" + is + "0"}, RouteSeparator)));
    }

    Y_UNIT_TEST(FromStringRoutesWithLeadingZeros) {
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle(JoinStrings({"1", "01234"}, RouteSeparator)));
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle(JoinStrings({"01", "1234"}, RouteSeparator)));
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle(JoinStrings({"01", "02", "1234"}, RouteSeparator)));
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle("3,014"));
    }

    Y_UNIT_TEST(LeadingPluses) {
        UNIT_ASSERT_EXCEPTION(TDocHandle(JoinStrings({"+7", "906", "123", "34", "56", "Z12345678DEADBEEF"}, RouteSeparator)), yexception);
        UNIT_ASSERT_EXCEPTION(TDocHandle(JoinStrings({"1", "+2", "3", "Z12345678DEADBEEF"}, RouteSeparator)), yexception);
        UNIT_ASSERT_EXCEPTION(TDocHandle("-47"), yexception);

        // The following cases are handled as string, though the real requirement is to be assertion-free.
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle("+47"));
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle("3,+14"));
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle("3,-14"));
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle("3,--14"));
        UNIT_ASSERT_NO_EXCEPTION(TDocHandle("3,-+14"));
    }

    Y_UNIT_TEST(Formats) {
        const auto s = TDocRoute::Separator;
        TString strHash = "Z12345678DEADBEEF";
        TString strHashWithRoute = TString("1") + s + "2" + s + "3" + s + strHash;
        TString strClientDocId = TString("2") + s + "3" + s + strHash;
        TString strClientDocIdIdx = strClientDocId + IndexDelimiter + "123";
        TString strHashWithIdx = strHash + IndexDelimiter + "123";
        TString strHashFull = strHashWithRoute + IndexDelimiter + "123";
        TDocHandle di(strHashFull);
        UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintAll), strHashFull);
        UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintRoute), strHashWithRoute);
        UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.AppendIndGeneration), strHashWithIdx);
        UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintHashOnly), strHash);
        UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintClientDocId), strClientDocId);
        UNIT_ASSERT_VALUES_EQUAL(di.ToString(di.PrintCliendDocIdWithIndex), strClientDocIdIdx);

        UNIT_ASSERT_VALUES_EQUAL(TDocHandle("ZABCD").ToString(TDocHandle::PrintClientDocId), "ZABCD");
        UNIT_ASSERT_VALUES_EQUAL(TDocHandle("Z12345678DEADBEEF").ToString(TDocHandle::PrintClientDocId), "Z12345678DEADBEEF");
        UNIT_ASSERT_VALUES_EQUAL(TDocHandle(TString("1") + s + "Z12345678DEADBEEF").ToString(TDocHandle::PrintClientDocId), "Z12345678DEADBEEF");
        // String fallback, may be removed once no longer needed
        UNIT_ASSERT_VALUES_EQUAL(TDocHandle(TString("1") + s + "2" + s + "hello").ToString(TDocHandle::PrintClientDocId), TString("2") + s + "hello");
    }

    Y_UNIT_TEST(ToStream) {
        const auto s = TDocRoute::Separator;
        TString strHash = "12345678DEADBEEF";
        TString strHashWithRoute = TString("1") + s + "2" + s + "3" + s + strHash;
        TString strHashWithIdx = strHash + IndexDelimiter + "123";
        auto check = [](const TString& s){
            TDocHandle di(s);
            TStringStream ss;
            ss << di;
            UNIT_ASSERT_VALUES_EQUAL(ss.Str(), s);
        };
        check(strHashWithIdx);
        check(strHashWithRoute);
        check(strHash);
        check("111");
        check("0");
        check("Z111");
        check("Z0");
    }

    Y_UNIT_TEST(Equals) {
        const NUrlId::TUrlId hashVal = NUrlId::Str2Hash(TStringBuf ("12345678DEADBEEF"));
        const auto s = TDocRoute::Separator;
        TString strZHash = TString("1") + s + "2" + s + "3" + s + "Z12345678DEADBEEF" + IndexDelimiter + "123";
        TString  strHash = TString("1") + s + "2" + s + "3" + s +  "12345678DEADBEEF" + IndexDelimiter + "123";
        TDocHandle fromStr(strZHash);
        TDocHandle nonZDocid(strHash);
        TDocHandle manual;
        UNIT_ASSERT_VALUES_UNEQUAL(manual, fromStr);
        UNIT_ASSERT_VALUES_UNEQUAL(fromStr, nonZDocid);
        manual.DocHash = hashVal;
        manual.PrependClientNum(3);
        manual.PrependClientNum(2);
        manual.PrependClientNum(1);
        manual.SetUnique();
        UNIT_ASSERT_VALUES_EQUAL(manual, fromStr);
    }

    Y_UNIT_TEST(BinarySaveLoad) {
        const auto s = TDocRoute::Separator;
        TString strHash = "Z12345678DEADBEEF";
        TString strHashWithRoute = TString("1") + s + "2" + s + "3" + s + strHash;
        TString strHashWithIdx = strHash + IndexDelimiter + "123";
        TString strHashFull = strHashWithRoute + IndexDelimiter + "123";
        auto check = [](const auto& s){
            TStringStream str;
            TDocHandle dh(s), loaded("Invalid");
            ::Save(&str, dh);
            ::Load(&str, loaded);
            UNIT_ASSERT_VALUES_EQUAL(dh, loaded);
        };
        check(strHash);
        check(strHashWithRoute);
        check(strHashWithIdx);
        check(strHashFull);
        check("Hello"); //< fallback base64-mode, remove once not needed anymore
        // Some tests for fully filled route
        check(TDocHandle(1234, {1,2,3,4,5,6}));
        check(TString("1") + s + "2" + s + "3" + s + "4" + s + "5" + s + "6" + s + "Z1234567890ABCDEF");
        check(TString("1") + s + "2" + s + "3" + s + "4" + s + "5" + s + "6" + s + "Hello"); //< fallback base64" + s + "mode, remove once not needed anymore
    }

    Y_UNIT_TEST(FormatZDocId) {
        const TString s = TString() + TDocRoute::Separator;
        {
            auto check = [](const TString& src, const TString& ref) {
                UNIT_ASSERT_VALUES_EQUAL(TDocHandle(src).ToString(TDocHandle::PrintZDocId), ref);
            };
            check("1" + s + "2" + s + "3" + s + "Z1234567890ABCDEF",
                                                "Z1234567890ABCDEF");
            check("1" + s + "2" + s + "3" + s + "1234567890ABCDEF",
                  "1" + s + "2" + s + "3" + s + "1234567890ABCDEF");
            check("Z1234567890ABCDEF",
                  "Z1234567890ABCDEF");
            // fallback base64-mode, remove once not needed anymore
            check("1" + s + "2" + s + "3" + s + "hello",
                  "1" + s + "2" + s + "3" + s + "hello");
        }
        {
            auto check = [](const TString& src, const TString& ref) {
                UNIT_ASSERT_VALUES_EQUAL(TDocHandle(src).ToString(TDocHandle::PrintZDocIdOmitZ), ref);
            };
            check("1" + s + "2" + s + "3" + s + "Z1234567890ABCDEF",
                                                 "1234567890ABCDEF");
            check("1" + s + "2" + s + "3" + s + "1234567890ABCDEF",
                  "1" + s + "2" + s + "3" + s + "1234567890ABCDEF");
            check("Z1234567890ABCDEF",
                   "1234567890ABCDEF");
            // fallback base64-mode, remove once not needed anymore
            check("1" + s + "2" + s + "3" + s + "hello",
                  "1" + s + "2" + s + "3" + s + "hello");
        }
    }
}
