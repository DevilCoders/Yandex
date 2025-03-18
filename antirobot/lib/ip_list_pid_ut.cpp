#include "ip_list_pid.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAntiRobot;

Y_UNIT_TEST_SUITE(TIpListProjIdTest) {
    static TIpListProjId MakeIpList(const TString& contents) {
        TIpListProjId ipList;
        TMemoryInput mi(contents.data(), contents.size());
        ipList.Load(mi);
        return ipList;
    }

    Y_UNIT_TEST(TestEmpty) {
        TIpListProjId ipList;
        UNIT_ASSERT(!ipList.Contains(TAddr()));
        UNIT_ASSERT(!ipList.Contains(TAddr("1.2.3.4")));
        UNIT_ASSERT(!ipList.Contains(TAddr("abcd::1234")));
    }

    Y_UNIT_TEST(TestSingle) {
        TIpListProjId ipList = MakeIpList("1.2.3.4\n");
        UNIT_ASSERT(!ipList.Contains(TAddr()));
        UNIT_ASSERT(ipList.Contains(TAddr("1.2.3.4")));
        UNIT_ASSERT(!ipList.Contains(TAddr("1.2.3.5")));
        UNIT_ASSERT(!ipList.Contains(TAddr("abcd::1234")));
    }

    Y_UNIT_TEST(TestProjectId) {
        TIpListProjId ipList = MakeIpList("4fe2@2a02:6b8:c00::/40\n");
        UNIT_ASSERT(!ipList.Contains(TAddr()));
        UNIT_ASSERT(!ipList.Contains(TAddr("2a02:6b8:c00::")));
        UNIT_ASSERT(!ipList.Contains(TAddr("2a02:6b8:c00:aa::")));
        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00::4fe2:00:00")));
        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00:0:0:4fe2::")));
        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00:aa:0:4fe2::")));
        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00:0:0:4fe2:aa::")));
    }


    Y_UNIT_TEST(TestProjectIdV4) {
        TIpListProjId ipList = MakeIpList("4fe2@150.3.4.0/16\n");
        UNIT_ASSERT(!ipList.Contains(TAddr()));
        UNIT_ASSERT(!ipList.Contains(TAddr("")));
        UNIT_ASSERT(!ipList.Contains(TAddr("150.3.4.1")));
        UNIT_ASSERT(!ipList.Contains(TAddr("150.3.4.12")));
    }

    Y_UNIT_TEST(TestProjectIdManyMasks) {
        TIpListProjId ipList = MakeIpList(
                "4fe2@2a02:6b8:c00::/40\n"
                "5fe2@2a02:6b8:c00::/40\n"
                );

        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00::4fe2:00:00")));
        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00:0:0:4fe2::")));
        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00:aa:0:4fe2::")));
        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00::5fe2:00:00")));
        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00:0:0:5fe2::")));
        UNIT_ASSERT(ipList.Contains(TAddr("2a02:6b8:c00:aa:0:5fe2::")));
    }

    Y_UNIT_TEST(TestProjectIdManyMasksException) {
        UNIT_ASSERT_EXCEPTION(
                MakeIpList(
                    "4fe2@2a02:6b8:c00::/40\n"
                    "5fe2@2a02:6b8::/32\n"
                    ),
                yexception
                );
    }

    Y_UNIT_TEST(TestSimple) {
        TIpListProjId ipList = MakeIpList(
            "5.6.7.8  # comment\n"
            "1.2.3.5\n"
            "12.34.56.78 another comment"
        );
        UNIT_ASSERT(ipList.Contains(TAddr("1.2.3.5")));
        UNIT_ASSERT(ipList.Contains(TAddr("5.6.7.8")));
        UNIT_ASSERT(ipList.Contains(TAddr("12.34.56.78")));
        UNIT_ASSERT(!ipList.Contains(TAddr("1.2.3.4")));
        UNIT_ASSERT(!ipList.Contains(TAddr("6.7.8.9")));
    }
}
