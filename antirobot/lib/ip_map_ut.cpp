#include <library/cpp/testing/unittest/registar.h>

#include "ip_map.h"

using namespace NAntiRobot;

namespace {
    void AddRangeToMap(TIpRangeMap<TString>& map,
                       const TString& rangeStart,
                       const TString& rangeEnd,
                       const TString& data) {
        map.Insert({TIpInterval{TAddr(rangeStart), TAddr(rangeEnd)}, data});
    }
}

Y_UNIT_TEST_SUITE(TestIpRangesMap) {
    Y_UNIT_TEST(TestFind) {
        TIpRangeMap<TString> map;

        AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");
        AddRangeToMap(map, "1.46.201.0", "1.46.201.0", "MEGAFON");
        AddRangeToMap(map, "1.46.225.0", "1.46.227.255", "TELE2");

        AddRangeToMap(map, "2405:205:630c:25ff:0000:0000:1a05:c800", "2405:205:630c:25ff:0000:0000:1a05:c800", "TELE2");
        AddRangeToMap(map, "2405:205:630c:4cb2:a894:0e1e:4852:9d00", "2405:205:630c:4cb2:a894:0e1e:4852:9dff", "MTS");
        AddRangeToMap(map, "2405:205:630c:9c6e:51e5:b9b0:ee6a:3700", "2405:205:630c:9c6e:51e5:b9b0:ee6a:37ff", "OTHER");

        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.192.0")).GetRef(), "OTHER");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.195.111")).GetRef(), "OTHER");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.199.255")).GetRef(), "OTHER");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.201.0")).GetRef(), "MEGAFON");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.225.0")).GetRef(), "TELE2");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.227.127")).GetRef(), "TELE2");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.227.255")).GetRef(), "TELE2");

        UNIT_ASSERT(map.Find(TAddr("1.46.191.255")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.200.0")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.200.127")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.200.255")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.201.1")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.224.255")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.228.0")).Empty());

        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:25ff:0000:0000:1a05:c800")).GetRef(), "TELE2");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:4cb2:a894:0e1e:4852:9d00")).GetRef(), "MTS");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:4cb2:a894:0e1e:4852:9dcc")).GetRef(), "MTS");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:4cb2:a894:0e1e:4852:9dff")).GetRef(), "MTS");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:9c6e:51e5:b9b0:ee6a:37ff")).GetRef(), "OTHER");

        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:25ff:0000:0000:1a05:c700")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:25ff:0000:0000:1a05:c801")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:35ff:aaaa:bbbb:cccc:dddd")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:4cb2:a894:0e1e:4852:9cff")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:4cb2:a894:0e1e:4852:9e00")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:9c6e:51e5:b9b0:ee6a:3800")).Empty());
    }

    Y_UNIT_TEST(TestFailWhenSameKeyWithDifferentValues) {
        TIpRangeMap<TString> map;
        AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");

        UNIT_ASSERT_EXCEPTION(AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "ANOTHER"), yexception);
    }

    Y_UNIT_TEST(TestNotFailWhenSameKeyWithSameValues) {
        TIpRangeMap<TString> map;
        AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");

        UNIT_ASSERT_NO_EXCEPTION(AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER"));
    }

    Y_UNIT_TEST(TestIntersections) {
        {
            TIpRangeMap<TString> map;
            AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");
            AddRangeToMap(map, "1.46.192.0", "1.46.193.0", "OTHER");

            UNIT_ASSERT_EXCEPTION(map.EnsureNoIntersections(), yexception);
        }

        {
            TIpRangeMap<TString> map;
            AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");
            AddRangeToMap(map, "1.46.196.0", "1.46.199.255", "OTHER");

            UNIT_ASSERT_EXCEPTION(map.EnsureNoIntersections(), yexception);
        }

        {
            TIpRangeMap<TString> map;
            AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");
            AddRangeToMap(map, "1.46.192.0", "1.46.192.0", "OTHER");

            UNIT_ASSERT_EXCEPTION(map.EnsureNoIntersections(), yexception);
        }

        {
            TIpRangeMap<TString> map;
            AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");
            AddRangeToMap(map, "1.46.199.255", "1.46.199.255", "OTHER");

            UNIT_ASSERT_EXCEPTION(map.EnsureNoIntersections(), yexception);
        }

        {
            TIpRangeMap<TString> map;
            AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");
            AddRangeToMap(map, "1.46.193.0", "1.46.196.0", "OTHER");

            UNIT_ASSERT_EXCEPTION(map.EnsureNoIntersections(), yexception);
        }

        {
            TIpRangeMap<TString> map;
            AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");
            AddRangeToMap(map, "1.46.193.0", "1.46.193.0", "OTHER");

            UNIT_ASSERT_EXCEPTION(map.EnsureNoIntersections(), yexception);
        }

        {
            TIpRangeMap<TString> map;
            AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");
            AddRangeToMap(map, "1.46.193.0", "1.46.200.255", "OTHER");

            UNIT_ASSERT_EXCEPTION(map.EnsureNoIntersections(), yexception);
        }

        {
            TIpRangeMap<TString> map;
            AddRangeToMap(map, "1.46.192.0", "1.46.199.255", "OTHER");
            AddRangeToMap(map, "1.46.199.255", "1.46.225.255", "OTHER");

            UNIT_ASSERT_EXCEPTION(map.EnsureNoIntersections(), yexception);
        }

        {
            TIpRangeMap<TString> map;
            AddRangeToMap(map, "2405:0205:630c:25ff::1a05:c800", "2405:0205:630c:25ff::3a05:c800", "TELE2");
            AddRangeToMap(map, "2405:0205:630c:25ff::2a05:c800", "2405:0205:630c:25ff::4a05:c800", "TELE2");

            UNIT_ASSERT_EXCEPTION(map.EnsureNoIntersections(), yexception);
        }
    }
}
