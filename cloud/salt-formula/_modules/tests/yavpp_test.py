import unittest

from yavpp import bgp2vpp_announce_prepare


class Bgp2vppTest(unittest.TestCase):

    def test_ipv4_not_specific(self):
        self.assertEqual(
            bgp2vpp_announce_prepare("172.16.0.176 via 10.255.254.93 label 3"),
            "172.16.0.176/32 label 3 nexthop 10.255.254.93 origin igp"
        )

    def test_ipv6(self):
        self.assertEqual(
            bgp2vpp_announce_prepare("2a02:6b8:c02:970::/64 via FE80::CD:C93"),
            "2a02:6b8:c02:970::/64 nexthop FE80::CD:C93 origin igp"
        )

    def test_ipv6_rt(self):
        self.assertEqual(
            bgp2vpp_announce_prepare("::/0 via ::ffff:10.0.97.93 rt 65533:776"),
            "::/0 nexthop ::ffff:10.0.97.93 rt 65533:776 origin igp"
        )

    def test_comm(self):
        self.assertEqual(
            bgp2vpp_announce_prepare("198.18.230.0/24 via 172.16.0.176 comm 65000:9003"),
            "198.18.230.0/24 nexthop 172.16.0.176 community 65000:9003 origin igp"
        )

    def test_default(self):
        self.assertEqual(
            bgp2vpp_announce_prepare("0.0.0.0/0 via 10.0.97.93 rt 65533:774"),
            "0.0.0.0/0 nexthop 10.0.97.93 rt 65533:774 origin igp"
        )

    def test_nat_not_supported(self):
        with self.assertRaises(NotImplementedError):
            bgp2vpp_announce_prepare("198.18.230.0/24 via nat")
