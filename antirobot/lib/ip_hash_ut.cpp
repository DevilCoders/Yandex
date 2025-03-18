#include <library/cpp/testing/unittest/registar.h>

#include "ip_hash.h"
#include "ip_vec.h"

namespace NAntiRobot {

namespace {
    template <typename T>
    void AddRangeToMap(T& map,
                       const TString& rangeStart,
                       const TString& rangeEnd,
                       const TString& data) {
        map.Insert({TIpInterval{TAddr(rangeStart), TAddr(rangeEnd)}, data});
    }

    template <typename T>
    void Test(T& map) {
        AddRangeToMap(map, "1.46.192.0", "1.46.192.255", "OTHER");
        AddRangeToMap(map, "1.46.201.0", "1.46.201.255", "MEGAFON");
        AddRangeToMap(map, "1.46.225.0", "1.46.225.255", "TELE2");

        AddRangeToMap(map, "2405:205:630c:25ff:0000:0000:0000:0000", "2405:205:630c:25ff:ffff:ffff:ffff:ffff", "TELE2");
        AddRangeToMap(map, "2405:205:630c:4cb2:0000:0000:0000:0000", "2405:205:630c:4cb2:ffff:ffff:ffff:ffff", "MTS");
        AddRangeToMap(map, "2405:205:630c:9c6e:0000:0000:0000:0000", "2405:205:630c:9c6e:ffff:ffff:ffff:ffff", "OTHER");
        map.Finish();

        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.192.0")).GetRef(), "OTHER");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.192.111")).GetRef(), "OTHER");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.192.255")).GetRef(), "OTHER");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.201.0")).GetRef(), "MEGAFON");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.225.0")).GetRef(), "TELE2");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.225.127")).GetRef(), "TELE2");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("1.46.225.255")).GetRef(), "TELE2");

        UNIT_ASSERT(map.Find(TAddr("1.46.191.255")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.200.0")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.200.127")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.200.255")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.202.1")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.224.255")).Empty());
        UNIT_ASSERT(map.Find(TAddr("1.46.228.0")).Empty());

        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:25ff:0000:0000:1a05:c800")).GetRef(), "TELE2");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:4cb2:a894:0e1e:4852:9d00")).GetRef(), "MTS");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:4cb2:a894:0e1e:4852:9dcc")).GetRef(), "MTS");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:4cb2:a894:0e1e:4852:9dff")).GetRef(), "MTS");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2405:205:630c:9c6e:51e5:b9b0:ee6a:37ff")).GetRef(), "OTHER");

        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:25fc:0000:0000:1a05:c700")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:25fc:0000:0000:1a05:c801")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:35ff:aaaa:bbbb:cccc:dddd")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:4cb3:a894:0e1e:4852:9cff")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:4cb4:a894:0e1e:4852:9e00")).Empty());
        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:9c6f:51e5:b9b0:ee6a:3800")).Empty());
    }

    template <typename T>
    void Test2(T& map) {
        AddRangeToMap(map, "2607:f1c0:630c:25ff:0000:0000:0000:0000", "2607:f1c0:630c:25ff:ffff:ffff:ffff:ffff", "TELE2");
        map.Finish();

        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2607:f1c0:640c:25ff:0000:0000:1a05:c800")).GetRef(), "TELE2");
        UNIT_ASSERT_STRINGS_EQUAL(map.Find(TAddr("2607:f1c0:630c:25fc:0000:0000:1a05:c700")).GetRef(), "TELE2");

        UNIT_ASSERT(map.Find(TAddr("2405:0205:630c:25fc:0000:0000:1a05:c801")).Empty());
    }
} // anonymous namespace

Y_UNIT_TEST_SUITE(TestHashAndVector) {
    Y_UNIT_TEST(TestFindInIpHashMap) {
        TIpHashMap<TString, 24, 64> map;
        Test(map);
    }

    Y_UNIT_TEST(TestFindInIpHashMap2) {
        TIpHashMap<TString, 24, 32> map;
        Test2(map);
    }

    Y_UNIT_TEST(TestFindInIpVector) {
        TIpVector<TString, 24, 64> map;
        Test(map);
    }

}

} // namespace NAntiRobot
