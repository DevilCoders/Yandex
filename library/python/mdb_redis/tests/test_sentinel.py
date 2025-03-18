# -*- coding: utf-8 -*-
import os
import pytest

from mdb_redis.sentinel import MdbSortedSlaves, MdbSentinelConnectionPool
from redis.sentinel import MasterNotFoundError, SlaveNotFoundError
from unittest.mock import patch


def test_mdb_sorted_slaves__init():
    os.environ.pop('DEPLOY_NODE_DC', None)
    mdb_slaves = MdbSortedSlaves()
    assert mdb_slaves.current_dc is None
    assert sorted(mdb_slaves.preferred_dcs) == sorted(list(mdb_slaves.PREFERRED_DCS.keys()))

    mdb_slaves = MdbSortedSlaves('sas')
    assert mdb_slaves.current_dc == 'sas'
    assert mdb_slaves.preferred_dcs == ('sas', 'vla', 'iva', 'myt', 'man')

    os.environ['DEPLOY_NODE_DC'] = 'sas'
    mdb_slaves = MdbSortedSlaves()
    assert mdb_slaves.current_dc == 'sas'
    assert mdb_slaves.preferred_dcs == ('sas', 'vla', 'iva', 'myt', 'man')

    os.environ['DEPLOY_NODE_DC'] = 'not'
    mdb_slaves = MdbSortedSlaves()
    assert mdb_slaves.current_dc == 'not'
    assert sorted(mdb_slaves.preferred_dcs) == sorted(list(mdb_slaves.PREFERRED_DCS.keys()))


def test_mdb_sorted_slaves__get_dc():
    mdb_slaves = MdbSortedSlaves()

    with patch('socket.getfqdn', return_value='vla-123.yandex.net'):
        assert mdb_slaves.get_dc('172.31.3.157') == 'vla'
        assert mdb_slaves.get_dc('2a02:6b8:b080:8000::1:1c') == 'vla'

    with patch('socket.getfqdn', return_value='cloud123.yandex.net'):
        assert mdb_slaves.get_dc('172.31.3.157') is None
        assert mdb_slaves.get_dc('2a02:6b8:b080:8000::1:1c') is None


def test_mdb_sorted_slaves__get_sorted_slaves():
    slaves = [
        ('2a02:6b8:b080:8000::1:1c', 5432),  # vla
        ('2a02:6b7:b070:7000::2:2c', 5432),  # sas
        ('2a02:6b6:b060:6000::3:3c', 5432),  # man
    ]

    mdb_slaves = MdbSortedSlaves('sas')
    with patch.object(mdb_slaves, 'get_dc', side_effect=['vla', 'sas', 'man']):
        assert mdb_slaves.get_sorted_slaves(slaves) == [
            ('2a02:6b7:b070:7000::2:2c', 5432),  # sas
            ('2a02:6b8:b080:8000::1:1c', 5432),  # vla
            ('2a02:6b6:b060:6000::3:3c', 5432),  # man
        ]

    mdb_slaves = MdbSortedSlaves('vla')
    with patch.object(mdb_slaves, 'get_dc', side_effect=['vla', 'sas', 'man']):
        assert mdb_slaves.get_sorted_slaves(slaves) == [
            ('2a02:6b8:b080:8000::1:1c', 5432),  # vla
            ('2a02:6b7:b070:7000::2:2c', 5432),  # sas
            ('2a02:6b6:b060:6000::3:3c', 5432),  # man
        ]

    mdb_slaves = MdbSortedSlaves('iva')
    with patch.object(mdb_slaves, 'get_dc', side_effect=['vla', 'sas', 'man']):
        assert mdb_slaves.get_sorted_slaves(slaves) == [
            ('2a02:6b8:b080:8000::1:1c', 5432),  # vla
            ('2a02:6b7:b070:7000::2:2c', 5432),  # sas
            ('2a02:6b6:b060:6000::3:3c', 5432),  # man
        ]

    mdb_slaves = MdbSortedSlaves('myt')
    with patch.object(mdb_slaves, 'get_dc', side_effect=['vla', 'sas', 'man']):
        assert mdb_slaves.get_sorted_slaves(slaves) == [
            ('2a02:6b8:b080:8000::1:1c', 5432),  # vla
            ('2a02:6b7:b070:7000::2:2c', 5432),  # sas
            ('2a02:6b6:b060:6000::3:3c', 5432),  # man
        ]

    mdb_slaves = MdbSortedSlaves('man')
    with patch.object(mdb_slaves, 'get_dc', side_effect=['vla', 'sas', 'man']):
        assert mdb_slaves.get_sorted_slaves(slaves) == [
            ('2a02:6b6:b060:6000::3:3c', 5432),  # man
            ('2a02:6b8:b080:8000::1:1c', 5432),  # vla
            ('2a02:6b7:b070:7000::2:2c', 5432),  # sas
        ]


class MockSentinel:
    def discover_master(self, service_name):
        pass


def test_mdb_sentinel_connection_pool__rotate_slaves():
    slaves = [
        ('2a02:6b8:b080:8000::1:1c', 5432),  # vla
        ('2a02:6b7:b070:7000::2:2c', 5432),  # sas
        ('2a02:6b6:b060:6000::3:3c', 5432),  # man
    ]
    master = ('2a02:6b5:b050:5000::4:4c', 5432)  # sas

    os.environ['DEPLOY_NODE_DC'] = 'sas'
    pool = MdbSentinelConnectionPool(service_name='test', sentinel_manager=MockSentinel())
    with (
        patch.object(pool, 'get_slaves', return_value=slaves),
        patch.object(pool.mdb_slaves, 'get_dc', side_effect=['vla', 'sas', 'man']),
    ):
        assert next(pool.rotate_slaves()) == ('2a02:6b7:b070:7000::2:2c', 5432)

    os.environ['DEPLOY_NODE_DC'] = 'myt'
    pool = MdbSentinelConnectionPool(service_name='test', sentinel_manager=MockSentinel())
    with (
        patch.object(pool, 'get_slaves', return_value=slaves),
        patch.object(pool.mdb_slaves, 'get_dc', side_effect=['vla', 'sas', 'man']),
    ):
        assert next(pool.rotate_slaves()) == ('2a02:6b8:b080:8000::1:1c', 5432)

    pool = MdbSentinelConnectionPool(service_name='test', sentinel_manager=MockSentinel())
    with (
        patch.object(pool, 'get_slaves', return_value=None),
        patch.object(pool, 'get_master_address', return_value=master),
    ):
        assert next(pool.rotate_slaves()) == ('2a02:6b5:b050:5000::4:4c', 5432)

    pool = MdbSentinelConnectionPool(service_name='test', sentinel_manager=MockSentinel())
    with (
        patch.object(pool, 'get_slaves', return_value=None),
        patch.object(pool, 'get_master_address', side_effect=MasterNotFoundError),
    ):
        with pytest.raises(SlaveNotFoundError):
            next(pool.rotate_slaves())
