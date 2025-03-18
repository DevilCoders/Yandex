#include <library/cpp/testing/unittest/registar.h>

#include "ip_interval.h"

using namespace NAntiRobot;

template <>
void Out<NAntiRobot::TIpInterval>(IOutputStream& out, const NAntiRobot::TIpInterval& range) {
    out << range.Print();
}

Y_UNIT_TEST_SUITE(TestIpInterval) {
    Y_UNIT_TEST_SUITE(ParseNetwork) {
        TIpInterval MakeRange(const TString& start, const TString& end) {
            TAddr startAddress(start);
            Y_ENSURE(startAddress.Valid(), "Cannot parse ip address: " << start);
            TAddr endAddress(end);
            Y_ENSURE(endAddress.Valid(), "Cannot parse ip address: " << end);
            return {startAddress, endAddress};
        }

        Y_UNIT_TEST(TestParseNetwork) {
            UNIT_ASSERT_VALUES_EQUAL(TIpInterval::Parse("0.0.0.0/32"), MakeRange("0.0.0.0", "0.0.0.0"));
            UNIT_ASSERT_VALUES_EQUAL(TIpInterval::Parse("0.0.0.0/16"), MakeRange("0.0.0.0", "0.0.255.255"));
            UNIT_ASSERT_VALUES_EQUAL(TIpInterval::Parse("0.0.111.222/16"), MakeRange("0.0.0.0", "0.0.255.255"));
            UNIT_ASSERT_VALUES_EQUAL(TIpInterval::Parse("0.0.0.0/0"), MakeRange("0.0.0.0", "255.255.255.255"));
            UNIT_ASSERT_VALUES_EQUAL(TIpInterval::Parse("1.2.3.4/5"), MakeRange("0.0.0.0", "7.255.255.255"));

            UNIT_ASSERT_EQUAL(TIpInterval::Parse("::1/128"), MakeRange("::1", "::1"));
            UNIT_ASSERT_EQUAL(TIpInterval::Parse("2a02:6b8:0:408:24e9:c3b0:a534:864/64"),
                              MakeRange("2a02:6b8:0:408::", "2a02:6b8:0:408:ffff:ffff:ffff:ffff"));
            UNIT_ASSERT_EQUAL(TIpInterval::Parse("2a02:6b8:0:408:24e9::a534:864/64"),
                              MakeRange("2a02:6b8:0:408::", "2a02:6b8:0:408:ffff:ffff:ffff:ffff"));
            UNIT_ASSERT_EQUAL(TIpInterval::Parse("2a02:6b8:0:408::a534:864/64"),
                              MakeRange("2a02:6b8:0:408::", "2a02:6b8:0:408:ffff:ffff:ffff:ffff"));
            UNIT_ASSERT_EQUAL(TIpInterval::Parse("2a02:6b8:0:408:24e9::a534:864/0"),
                              MakeRange("::", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
        }

        Y_UNIT_TEST(TestParseNetworkFail) {
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("0.0.0.0/33"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("0.0.0.0/-1"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("0.0.0.0/sdfjns"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("0.0.0.0/"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("0.0.0.0.0/32"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("0.0.0/32"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("0.256.0.0/32"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("0.0.0.-1/32"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("0.0.sdfsd.0/32"), yexception);

            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("2a02:6b8:0:408:24e9:c3b0:a534:864/129"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("2a02:6b8:0:408:24e9:c3b0:a534:864/-1"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("2a02:6b8:0:408:24e9:c3b0:a534:/64"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("2a02:6b8:0:408:24e9:c3b0/64"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("2a02:6b8:0:408:24e9:c3b0:a534:864:1234/64"), yexception);
            UNIT_ASSERT_EXCEPTION(TIpInterval::Parse("2a02:6b8:0:408:124e9:c3b0:a534:864/64"), yexception);
        }
    }
}
