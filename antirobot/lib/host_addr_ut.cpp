#include <library/cpp/testing/unittest/registar.h>

#include "addr.h"
#include "host_addr.h"

#include <util/generic/set.h>
#include <util/system/hostname.h>

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(TestHostAddr)
{

Y_UNIT_TEST(TestConstructor) {
    TString hostName = FQDNHostName();
    UNIT_ASSERT_STRINGS_EQUAL(ToString(CreateHostAddr(hostName)), hostName);
    UNIT_ASSERT_STRINGS_EQUAL(CreateHostAddr(hostName).HostName, hostName);
    UNIT_ASSERT_EQUAL(CreateHostAddr(hostName).Port, 80);

    TString hostNameWithPort = hostName + ":8888";
    UNIT_ASSERT_STRINGS_EQUAL(ToString(CreateHostAddr(hostNameWithPort)), hostName);
    UNIT_ASSERT_STRINGS_EQUAL(CreateHostAddr(hostNameWithPort).HostName, hostName);
    UNIT_ASSERT_EQUAL(CreateHostAddr(hostNameWithPort).Port, 8888);
}

Y_UNIT_TEST(TestWrongHost) {
    UNIT_ASSERT_EXCEPTION(CreateHostAddr("be-be-be"), TNetworkResolutionError);
}

TSet<TAddr> GetIps(const TNetworkAddress& addr) {
    TSet<TAddr> res;
    for (TNetworkAddress::TIterator i = addr.Begin(); i != addr.End(); ++i) {
        res.insert(TAddr(i->ai_addr));
    }
    return res;
}

TString IpsToString(TSet<TAddr> ips) {
    TStringBuilder sb;
    for (auto ip : ips) {
        sb << ip.ToString() << " ";
    }
    return sb;
}

Y_UNIT_TEST(TestNetworkAddressIdentity) {
    const TString HOSTS[] = {
        "localhost",
        HostName(),
        FQDNHostName(),
        "any.yandex.ru", // Remove it if test will be flaky again
    };

    for (size_t k = 0; k < Y_ARRAY_SIZE(HOSTS); ++k) {
        //
        // try to get it cached, then check for equality
        //
        TNetworkAddress dummy1(HOSTS[k], 80);
        THostAddr dummy2(CreateHostAddr(HOSTS[k]));
        Y_UNUSED(dummy1);
        Y_UNUSED(dummy2);

        TSet<TAddr> ips1 = GetIps(TNetworkAddress(HOSTS[k], 80));
        TSet<TAddr> ips2 = GetIps(CreateHostAddr(HOSTS[k]));
        UNIT_ASSERT_VALUES_EQUAL_C(ips1.size(), ips2.size(), TString("for host: ") + HOSTS[k]);
        UNIT_ASSERT_C(std::equal(ips1.begin(), ips1.end(), ips2.begin()),
            TString("for host: ") + HOSTS[k] + " " + IpsToString(ips1) + "; " + IpsToString(ips2));
    }
}

}

}
