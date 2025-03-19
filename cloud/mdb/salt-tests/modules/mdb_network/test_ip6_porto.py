# -*- coding: utf-8 -*-

from cloud.mdb.salt.salt._modules import mdb_network
from cloud.mdb.salt_tests.common.mocks import mock_grains
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'single address',
        'args': {
            'grains': {
                'porto_resources': {
                    'container': {'ip': 'eth0 2a02:6b8:c08:aa2c:0:1589:bb71:b9f3'},
                },
            },
            'user_addr': '2a02:6b8:c08:aa2c:0:1589:bb71:b9f3',
            'control_addr': '2a02:6b8:c08:aa2c:0:1589:bb71:b9f3',
        },
    },
    {
        'id': 'dual address',
        'args': {
            'grains': {
                'porto_resources': {
                    'container': {
                        'ip': 'eth0 2a02:6b8:c08:aa2c:0:5703:ea3b:948f;eth0 2a02:6b8:c08:aa2c:0:1589:bb71:b9f3'
                    },
                },
            },
            'user_addr': '2a02:6b8:c08:aa2c:0:5703:ea3b:948f',
            'control_addr': '2a02:6b8:c08:aa2c:0:1589:bb71:b9f3',
        },
    },
    {
        'id': 'dual address (reverse)',
        'args': {
            'grains': {
                'porto_resources': {
                    'container': {
                        'ip': 'eth0 2a02:6b8:c08:aa2c:0:1589:bb71:b9f3;eth0 2a02:6b8:c08:aa2c:0:5703:ea3b:948f'
                    },
                },
            },
            'user_addr': '2a02:6b8:c08:aa2c:0:1589:bb71:b9f3',
            'control_addr': '2a02:6b8:c08:aa2c:0:5703:ea3b:948f',
        },
    },
)
def test_ip6_porto(grains, user_addr, control_addr):
    mock_grains(mdb_network.__salt__, grains)
    assert mdb_network.ip6_porto_user_addr() == user_addr
    assert mdb_network.ip6_porto_control_addr() == control_addr
