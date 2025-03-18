#include <library/cpp/testing/unittest/registar.h>

#include "addr.h"
#include "ar_utils.h"
#include "ip_list.h"

#include <util/stream/mem.h>

namespace NAntiRobot {
    Y_UNIT_TEST_SUITE(TTestIpList) {
        static TIpList MakeIpList(const TString& contents) {
            TIpList ipList;
            TMemoryInput mi(contents.data(), contents.size());
            ipList.Load(mi);
            return ipList;
        }

        Y_UNIT_TEST(TestEmpty) {
            TIpList ipList;
            UNIT_ASSERT(!ipList.Contains(TAddr()));
            UNIT_ASSERT(!ipList.Contains(TAddr("1.2.3.4")));
        }

        Y_UNIT_TEST(TestSingle) {
            TIpList ipList = MakeIpList("1.2.3.4\n");
            UNIT_ASSERT(!ipList.Contains(TAddr()));
            UNIT_ASSERT(ipList.Contains(TAddr("1.2.3.4")));
            UNIT_ASSERT(!ipList.Contains(TAddr("1.2.3.5")));
        }

        Y_UNIT_TEST(TestSimple) {
            TIpList ipList = MakeIpList("5.6.7.8  # comment\n1.2.3.5\n12.34.56.78 another comment");
            UNIT_ASSERT(ipList.Contains(TAddr("1.2.3.5")));
            UNIT_ASSERT(ipList.Contains(TAddr("5.6.7.8")));
            UNIT_ASSERT(ipList.Contains(TAddr("12.34.56.78")));
            UNIT_ASSERT(!ipList.Contains(TAddr("1.2.3.4")));
            UNIT_ASSERT(!ipList.Contains(TAddr("6.7.8.9")));
        }

        Y_UNIT_TEST(TestSingleV6) {
            TIpList ipList = MakeIpList("abcd::1234\n");
            UNIT_ASSERT(!ipList.Contains(TAddr()));
            UNIT_ASSERT(ipList.Contains(TAddr("abcd::1234")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abcd::1235")));
        }

        Y_UNIT_TEST(TestSomeIntervals) {
            TIpList ipList = MakeIpList(
                "100.1.2.3 - 100.1.2.30\n"
                "200.1.2.3 - 200.1.2.30  \t#comment\n"
                "100.1.2.20\n"
                "100.1.2.10\n"
                "100.2.2.0/24\n"
                "200.1.2.29 - 200.1.2.40   comment\n"
                "150.3.4.5 - 150.3.4.100  \tcomment\n"
                "150.3.4.101 - 150.3.4.200\n"
                "150.3.4.201\n"
                "150.3.4.203\n"
                "abc0::1234\n"
                "abca::1234 - abca::5678\n"
                "abcb::1234:5678 - abcb::1244:6789  comment\n"
                "abcc::1234:0/112  \tcomment\n"
                "abcd:1234:cdef::/32  \t#comment\n"
                "abce:2345:5678::/36#comment\n"
                );

            UNIT_ASSERT(!ipList.Contains(TAddr()));

            UNIT_ASSERT(!ipList.Contains(TAddr("100.1.2.2")));
            UNIT_ASSERT(ipList.Contains(TAddr("100.1.2.3")));
            UNIT_ASSERT(ipList.Contains(TAddr("100.1.2.20")));
            UNIT_ASSERT(ipList.Contains(TAddr("100.1.2.21")));
            UNIT_ASSERT(ipList.Contains(TAddr("100.1.2.30")));
            UNIT_ASSERT(!ipList.Contains(TAddr("100.1.2.31")));

            UNIT_ASSERT(!ipList.Contains(TAddr("100.2.1.255")));
            UNIT_ASSERT(ipList.Contains(TAddr("100.2.2.0")));
            UNIT_ASSERT(ipList.Contains(TAddr("100.2.2.50")));
            UNIT_ASSERT(ipList.Contains(TAddr("100.2.2.200")));
            UNIT_ASSERT(ipList.Contains(TAddr("100.2.2.255")));
            UNIT_ASSERT(!ipList.Contains(TAddr("100.2.3.1")));

            UNIT_ASSERT(!ipList.Contains(TAddr("200.1.2.2")));
            UNIT_ASSERT(ipList.Contains(TAddr("200.1.2.3")));
            UNIT_ASSERT(ipList.Contains(TAddr("200.1.2.28")));
            UNIT_ASSERT(ipList.Contains(TAddr("200.1.2.29")));
            UNIT_ASSERT(ipList.Contains(TAddr("200.1.2.30")));
            UNIT_ASSERT(ipList.Contains(TAddr("200.1.2.31")));
            UNIT_ASSERT(ipList.Contains(TAddr("200.1.2.40")));
            UNIT_ASSERT(!ipList.Contains(TAddr("200.1.2.41")));

            UNIT_ASSERT(!ipList.Contains(TAddr("150.3.4.4")));
            UNIT_ASSERT(ipList.Contains(TAddr("150.3.4.5")));
            UNIT_ASSERT(ipList.Contains(TAddr("150.3.4.99")));
            UNIT_ASSERT(ipList.Contains(TAddr("150.3.4.100")));
            UNIT_ASSERT(ipList.Contains(TAddr("150.3.4.101")));
            UNIT_ASSERT(ipList.Contains(TAddr("150.3.4.102")));
            UNIT_ASSERT(ipList.Contains(TAddr("150.3.4.200")));
            UNIT_ASSERT(ipList.Contains(TAddr("150.3.4.201")));
            UNIT_ASSERT(!ipList.Contains(TAddr("150.3.4.202")));
            UNIT_ASSERT(ipList.Contains(TAddr("150.3.4.203")));
            UNIT_ASSERT(!ipList.Contains(TAddr("150.3.4.204")));

            // "abc0::1234\n"
            UNIT_ASSERT(!ipList.Contains(TAddr("abc0::1233")));
            UNIT_ASSERT(ipList.Contains(TAddr("abc0::1234")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abc0::1233")));

            // "abca::1234 - abca::5678\n"
            UNIT_ASSERT(!ipList.Contains(TAddr("abca::1233")));
            UNIT_ASSERT(ipList.Contains(TAddr("abca::1234")));
            UNIT_ASSERT(ipList.Contains(TAddr("abca::1235")));
            UNIT_ASSERT(ipList.Contains(TAddr("abca::4567")));
            UNIT_ASSERT(ipList.Contains(TAddr("abca::5677")));
            UNIT_ASSERT(ipList.Contains(TAddr("abca::5678")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abca::5679")));
            UNIT_ASSERT(!ipList.Contains(TAddr("bbca::1234")));

            // "abcb::1234:5678 - abcb::1244:6789  comment\n"
            UNIT_ASSERT(!ipList.Contains(TAddr("abcb::1234:5677")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcb::1234:5678")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcb::1234:5679")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcb::1234:ffff")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcb::1243:9876")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcb::1244:9"))); // prevent comparing addresses as printable strings
            UNIT_ASSERT(ipList.Contains(TAddr("abcb::1244:6788")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcb::1244:6789")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abcb::1244:6790")));
            UNIT_ASSERT(!ipList.Contains(TAddr("bbcb::1244:6789")));

            // "abcc::1234:0/112  \tcomment\n"
            UNIT_ASSERT(!ipList.Contains(TAddr("abcc::1233:ffff")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcc::1234:0")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcc::1234:1")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcc::1234:ffff")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abcc::1235:0")));
            UNIT_ASSERT(!ipList.Contains(TAddr("bbcc::1234:0")));

            // "abcd:1234:cdef::/32  \t#comment\n"
            UNIT_ASSERT(!ipList.Contains(TAddr("abcd:1233:ffff:ffff:ffff:ffff:ffff:ffff")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcd:1234:0000:0000:0000:0000:0000:0000")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcd:1234:0:0:0:0:0:0")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcd:1234::")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcd:1234::1")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcd:1234:abcd:ffff:1234:ffff:ffff:ffff")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcd:1234:ffff:ffff:ffff:ffff:ffff:fffe")));
            UNIT_ASSERT(ipList.Contains(TAddr("abcd:1234:ffff:ffff:ffff:ffff:ffff:ffff")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abcd:1235::")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abcd:1235:0:0:0:0:0:0")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abcd:1235::1")));
            UNIT_ASSERT(!ipList.Contains(TAddr("bbcd:1234::")));

            // "abce:2345:5678::/36#comment\n"
            UNIT_ASSERT(!ipList.Contains(TAddr("abce:2345:4000::")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abce:2345:4fff:ffff:ffff:ffff:ffff:ffff")));
            UNIT_ASSERT(ipList.Contains(TAddr("abce:2345:5000:0000:0000:0000:0000:0000")));
            UNIT_ASSERT(ipList.Contains(TAddr("abce:2345:5000:0:0:0:0:0")));
            UNIT_ASSERT(ipList.Contains(TAddr("abce:2345:5000::")));
            UNIT_ASSERT(ipList.Contains(TAddr("abce:2345:5000::1")));
            UNIT_ASSERT(ipList.Contains(TAddr("abce:2345:5abc:ffff:1234:ffff:ffff:ffff")));
            UNIT_ASSERT(ipList.Contains(TAddr("abce:2345:5fff:ffff:ffff:ffff:ffff:fffe")));
            UNIT_ASSERT(ipList.Contains(TAddr("abce:2345:5fff:ffff:ffff:ffff:ffff:ffff")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abce:2345:6000::")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abce:2346:6000:0:0:0:0:0")));
            UNIT_ASSERT(!ipList.Contains(TAddr("abce:2346:6000::1")));
            UNIT_ASSERT(!ipList.Contains(TAddr("bbce:2345:5000::")));
        }

        Y_UNIT_TEST(Glue) {
            TIpList ipList = MakeIpList(
                "150.3.4.201\n"
                "150.3.4.5 - 150.3.4.100\n"
                "150.3.4.101 - 150.3.4.200\n"
                "150.3.4.203\n"
                "abca::5679\n"
                "abca::1234 - abca::5678\n"
                "abcd::/16\n"
                "abce::\n"
                );

            UNIT_ASSERT_EQUAL(ipList.Size(), 4);
        }
    };
}
