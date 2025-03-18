from devtools.fleur.ytest import suite, test, TestSuite, generator
from devtools.fleur.ytest import AssertEqual, AssertNotEqual

from antirobot.scripts.utils import ip_utils

@suite(package="antirobot.scripts.utils")
class IpUtils(TestSuite):

    @generator([
        ('192.168.1.0/24', '192.168.1.0', True),
        ('192.168.1.0/24', '192.168.1.1', True),
        ('192.168.1.0/24', '192.168.1.255', True),
        ('192.168.1.0/24', '192.168.2.255', False),

        ('192.168.2.255/23', '192.168.2.0', True),
        ('192.168.2.255/23', '192.168.2.1', True),
        ('192.168.2.255/23', '192.168.3.23', True),

        ('192.168.2.255/23', '192.168.2.196', True),
        ('192.168.2.255/23', '192.168.2.1', True),
        ('192.168.2.255/23', '192.168.2.195', True),
        ('192.168.2.255/23', '192.168.2.197', True),

        ('92.38.240.0/20', '220.174.249.2', False),
        ])
    def MatchIpStr(self, netStr, ipStr, result):
        AssertEqual(ip_utils.Net(netStr).MatchIpStr(ipStr), result)

    @test
    def IpInList(self):
        ipRange = "192.168.1.2 - 192.168.1.10"
        ipList = ip_utils.IpList([ipRange], True)

        AssertNotEqual(ipList.IpInList("192.168.1.2"), None)
        AssertEqual(ipList.IpInList("192.168.1.11"), None)

        ipList = ip_utils.IpList(["192.168.1.10"])
        AssertNotEqual(ipList.IpInList("192.168.1.10"), None)
        AssertEqual(ipList.IpInList(12345678901234567890), None)

    @generator([
        ('1:2:123:1:1230:3215:1230:abcd', '0001:0002::0123:0001:1230:3215:1230:ABCD'),
        ('0:2:123:1:1230:3215:1230:abcd', '0000:0002::0123:0001:1230:3215:1230:ABCD'),
        ('0:2:123::3215:1230:abcd', '0000:0002::0123:000:0:3215:1230:ABCD'),
        ('0:1::1:1', '000:0001:00:00:00:00:001:001'),
        ('::1:1', '000:000:00:00:00:00:001:001'),
        ('::1:0:0:1:1:1', '000:000:01:00:00:01:001:001'),
        ('::', '000:000:0:00:00:0:00:000'),
        ('0:0:1::1:1', '000:000:01:00:00:00:001:001'),
        ('1.2.3.123', '001.002.03.123'),
        ('1:2:3::', '001:002:03:0:0:0:0:0'),
        ('1:2:3::', '001:002:03:0:0::'),
        ])
    def NormalizeIpString(self, normalizedIpStr, someIpStr):
        AssertEqual(ip_utils.NormalizeIpStr(someIpStr), normalizedIpStr)

    @generator([
        ('::1', '0:0:0:0:0:0:0:1'),
        ('1::1', '1:0:0:0:0:0:0:1'),
        ('1:0:0:1::', '1:0:0:1:0:0:0:0'),
        ('1F::', '1f:0:0:0:0:0:0:0'),
        ])
    def ExpandIpv6Str(self, ipv6Str, result):
        AssertEqual(ip_utils.ExpandIpv6Str(ipv6Str), result)

    @generator([
        ('1.2.3.4', '1.2.3.0'),
        ('1:2:3:4:5:6:7:8', '1:2:3:4:5:6::'),
        ('::1:2', '::'),
        ('1:0:0:0:0:0:0:1', '1::'),
        ('1:0:0:0:0:0:1:0', '1::'),
        ('1:0:0:0:0:1:1:0', '1::1:0:0'),
        ])
    def IpClassC(self, ipStr, result):
        AssertEqual(ip_utils.StrIpClassC(ipStr), result)

    @generator([
        ('1.2.3.4', '1.2.0.0'),
        ('1:2:3:4:5:6:7:8', '1:2:3:4::'),
        ('::1:2', '::'),
        ('1:0:0:0:0:0:0:1', '1::'),
        ('1:0:0:0:0:0:1:0', '1::'),
        ('1:0:0:0:0:1:1:0', '1::'),
        ('1:0:0:0:1:1:1:0', '1::'),
        ('1:0:0:1:1:1:1:0', '1:0:0:1::'),
        ('1:0:0:1::', '1:0:0:1::'),
        ('1:0:1:1:1:1:1:0', '1:0:1:1::'),
        ('::1:1:1:1:1:0', '0:0:1:1::'),
        ])
    def IpClassB(self, ipStr, result):
        AssertEqual(ip_utils.StrIpClassB(ipStr), result)
