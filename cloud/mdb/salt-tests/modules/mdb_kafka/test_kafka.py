# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_kafka
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_vtype
from cloud.mdb.internal.python.pytest.utils import parametrize
from mock import patch, MagicMock


@parametrize(
    {
        'id': 'count of brokers 3',
        'args': {'pillar': {'data': {'kafka': {'nodes': {'fqdn-1': {}, 'fqdn-2': {}, 'fqdn-3': {}}}}}, 'result': 3},
    },
    {'id': 'count of brokers 1', 'args': {'pillar': {'data': {'kafka': {'nodes': {'fqdn-1': {}}}}}, 'result': 1}},
)
def test_count_of_brokers(pillar, result):
    mock_pillar(mdb_kafka.__salt__, pillar)
    assert mdb_kafka.count_of_brokers() == result


@parametrize(
    {
        'id': 'count of brokers 3',
        'args': {
            'pillar': {'data': {'kafka': {'nodes': {'fqdn-1': {}, 'fqdn-2': {}, 'fqdn-3': {}}}}},
            'result': 'fqdn-1:9091,fqdn-2:9091,fqdn-3:9091',
        },
    },
    {
        'id': 'count of brokers 1',
        'args': {'pillar': {'data': {'kafka': {'nodes': {'fqdn-1': {}}}}}, 'result': 'fqdn-1:9091'},
    },
)
def test_kafka_connect_cluster_brokers_list(pillar, result):
    mock_pillar(mdb_kafka.__salt__, pillar)
    assert mdb_kafka.kafka_connect_cluster_brokers_list() == result


@parametrize(
    {
        'id': 'count of brokers 3',
        'args': {
            'pillar': {'data': {'kafka': {'nodes': {'fqdn-1': {}, 'fqdn-2': {}, 'fqdn-3': {}}}}},
            'result': 'fqdn-1,fqdn-2,fqdn-3',
        },
    },
    {
        'id': 'count of brokers 1',
        'args': {'pillar': {'data': {'kafka': {'nodes': {'fqdn-1': {}}}}}, 'result': 'fqdn-1'},
    },
)
def test_kafka_fqdns(pillar, result):
    mock_pillar(mdb_kafka.__salt__, pillar)
    assert mdb_kafka.kafka_fqdns() == result


@parametrize(
    {
        'id': 'when has_zk_subcluster is false and no zk_connect should return localhost:2181',
        'args': {
            'pillar': {'data': {'dbaas': {'vtype': 'compute'}, 'kafka': {'has_zk_subcluster': False}}},
            'result': 'localhost:2181',
        },
    },
    {
        'id': 'when has_zk_subcluster is false and zk_connect is defined should return localhost:2181',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {'vtype': 'compute'},
                    'kafka': {'has_zk_subcluster': False, 'zk_connect': 'fqdn-zk:2181'},
                }
            },
            'result': 'fqdn-zk:2181',
        },
    },
    {
        'id': 'when has_zk_subcluster is true should return connection from zk nodes',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {'vtype': 'compute'},
                    'kafka': {'has_zk_subcluster': True},
                    'zk': {'nodes': {'fqdn-1': {}, 'fqdn-2': {}, 'fqdn-3': {}}},
                }
            },
            'result': 'fqdn-1:2181,fqdn-2:2181,fqdn-3:2181',
        },
    },
)
def test_zk_connect(pillar, result):
    mock_pillar(mdb_kafka.__salt__, pillar)
    mock_vtype(mdb_kafka.__salt__, pillar['data']['dbaas']['vtype'])
    assert mdb_kafka.zk_connect() == result


@parametrize(
    {
        'id': 'when has_zk_subcluster is false should return empty string',
        'args': {
            'pillar': {
                'data': {
                    'kafka': {
                        'has_zk_subcluster': False,
                    }
                }
            },
            'result': '',
        },
    },
    {
        'id': 'when has_zk_subcluster is true should return connection from zk nodes',
        'args': {
            'pillar': {
                'data': {
                    'kafka': {'has_zk_subcluster': True},
                    'zk': {'nodes': {'fqdn-1': {}, 'fqdn-2': {}, 'fqdn-3': {}}},
                }
            },
            'result': 'fqdn-1,fqdn-2,fqdn-3',
        },
    },
)
def test_zk_fqdns(pillar, result):
    mock_pillar(mdb_kafka.__salt__, pillar)
    assert mdb_kafka.zk_fqdns() == result


def test_check_zk_host():
    mock_pillar(mdb_kafka.__salt__, {'data': {'dbaas': {'vtype': 'compute'}}})
    with patch('socket.socket') as mock_socket_class:
        mock_socket = MagicMock()
        mock_socket_class.return_value = mock_socket
        mdb_kafka.check_zk_host('host')
        mock_socket.settimeout.assert_called_once_with(3)
        mock_socket.connect.assert_called_once_with(('host', 2181))
        mock_socket.send.assert_called_once_with(b'ruok')
        mock_socket.recv.assert_called_once_with(4)
