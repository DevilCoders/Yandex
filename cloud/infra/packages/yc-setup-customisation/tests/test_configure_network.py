#!/usr/bin/env python3
import sys
from unittest import mock, TestCase

from parameterized import parameterized
from bin import configure_network

sys.modules["netifaces"] = mock.Mock()


class TestNetworkUdevChanger(TestCase):

    def mock_ifaddresses(self, iface):
        return {
            "eth0": {
                "10": [{"addr": "2a02:aaaa:bbbb:aaaa:bbbb:aaaa:bbbb:aaaa", "netmask": "ffff:ffff:ffff:ffff::"}]
            },
            "eth1": {
                "12": [{"addr": "fc01:aaaa:bbbb:aaaa:bbbb:aaaa:bbbb:aaaa", "netmask": "ffff:ffff:ffff:ffff::"}]
            }
        }.get(iface)

    @parameterized.expand([
        ("2a02", "2a0206b8bf002009526b4bfffedb7219 03 40 00 00 eth0\n", 1),
        ("2a0d", "2a0d06b8bf002009526b4bfffedb7219 03 40 00 00 eth0\n", 1),
        ("fc01", "fc0106b8bf002009526b4bfffedb7219 03 40 00 00 eth0\n", 2),
        ("link-local", "fe80000000000000526b4bfffedb7219 03 40 20 80\n", 3),
        ("lo", "00000000000000000000000000000001 01 80 10 80 lo", 3),
    ])
    def test__get_iface_priority_by_ip(self, case_name, file_content, expected_result):
        iface = configure_network.Interface("eth0")
        with mock.patch("builtins.open", mock.mock_open(read_data=file_content)):
            self.assertEqual(iface._get_ip_priority(), expected_result)

    @parameterized.expand([
        ("duplicate iface",
         "2a0206b8bf002009526b4bfffedb7219 03 40 00 00 eth0\n2a02aaaaaaaaaaaaaaaaaaaaaaaaaaaa 03 40 00 00 eth0\n",
         RuntimeError),
        ("unknown prefix",
         "5a0206b8bf002009526b4bfffedb7219 03 40 00 00 eth0\n",
         KeyError),
    ])
    def test__get_iface_priority_by_ip_duplicates(self, case_name, file_content, exception):
        iface = configure_network.Interface("eth0")
        with mock.patch("builtins.open", mock.mock_open(read_data=file_content)):
            with self.assertRaises(exception):
                iface._get_ip_priority()

    @parameterized.expand([
        ("ens", ["lo", "ens506b4bdb", "ens506b4bdb"], []),
        ("eth", ["lo", "eth0", "eth1", "trash"], ["eth0", "eth1"]),
        ("ens & eth", ["lo", "ens506b4bdb", "ens506b4bdb", "eth0", "eth1"], ["eth0", "eth1"]),
        ("none good", ["lo", "trash"], [])
    ])
    def test_get_good_ifaces(self, case_name, ifaces, expected_result):
        with mock.patch.object(configure_network, "list_interfaces") as mock_ifaces:
            mock_ifaces.return_value = ifaces
            names = [iface.name for iface in configure_network.get_good_ifaces()]
            self.assertEqual(names, expected_result)
