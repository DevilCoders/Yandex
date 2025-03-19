# -*- coding: utf-8 -*-

from cloud.mdb.salt.salt._modules import mdb_network
from cloud.mdb.salt_tests.common.mocks import mock_grains
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'ip6_for_interface_valid',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f806:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'args': ['eth0'],
            'result': '2a02:6b8:c0e:501:0:f806:0:283',
        },
    },
    {
        'id': 'ip6_for_interface_invalid_iface',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f806:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'args': ['eth1'],
            'result': None,
        },
    },
    {
        'id': 'ip6_for_interface_no_addr',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'args': ['eth0'],
            'result': None,
        },
    },
)
def test_ip6_for_interface(grains, args, result):
    mock_grains(mdb_network.__salt__, grains)
    assert mdb_network.ip6_for_interface(*args) == result


@parametrize(
    {
        'id': 'ip6_interfaces_valid',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        '2a02:6b8:c0e:501:0:f806:0:283',
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                    'eth1': [
                        'fe80::d21d:15ff:fe44:955d',
                        '2a02:6b8:c0e:501:0:f804:0:20e',
                    ],
                    'lo': [
                        '::1',
                    ],
                },
            },
            'result': {'eth0': '2a02:6b8:c0e:501:0:f806:0:283', 'eth1': '2a02:6b8:c0e:501:0:f804:0:20e'},
        },
    },
    {
        'id': 'ip6_interfaces_no_addr',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                    'eth1': [
                        '2a02:6b8:c0e:501:0:f804:0:20e',
                        'fe80::d21d:15ff:fe44:955d',
                    ],
                    'lo': [
                        '::1',
                        '2a02:6b8:c0e:501:0:f806:0:283',
                    ],
                },
            },
            'result': {'eth1': '2a02:6b8:c0e:501:0:f804:0:20e'},
        },
    },
    {
        'id': 'ip6_interfaces_bad_addr',
        'args': {
            'grains': {
                'ip6_interfaces': {
                    'eth0': [
                        'fe80::d20d:15ff:fe44:955d',
                        'fd01:ffff:ffff:ffff::2',
                    ],
                    'eth1': [
                        '2a02:6b8:c0e:501:0:f804:0:20e',
                        'fe80::d21d:15ff:fe44:955d',
                    ],
                    'lo': [
                        '::1',
                        '2a02:6b8:c0e:501:0:f806:0:283',
                    ],
                },
            },
            'result': {'eth1': '2a02:6b8:c0e:501:0:f804:0:20e'},
        },
    },
    {
        'id': 'ip6_interfaces_no_interfaces',
        'args': {
            'grains': {
                'ip6_interfaces': {},
            },
            'result': {},
        },
    },
)
def test_ip6_interfaces(grains, result):
    mock_grains(mdb_network.__salt__, grains)
    assert mdb_network.ip6_interfaces() == result
