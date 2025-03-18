# -*- coding: utf-8 -*-
import os
import pytest

from mdb_redis.aiosentinel import MdbSentinelConnectionPool, MdbSentinel
from redis.sentinel import MasterNotFoundError, SlaveNotFoundError
from unittest.mock import patch, AsyncMock


class MockSentinel:
    def discover_master(self, service_name):
        pass


async def get_one(async_generator):
    async for obj in async_generator:
        return obj


@pytest.mark.asyncio
async def test_mdb_sentinel_connection_pool__rotate_slaves():
    slaves = AsyncMock(return_value=[
        ('2a02:6b8:b080:8000::1:1c', 5432),  # vla
        ('2a02:6b7:b070:7000::2:2c', 5432),  # sas
        ('2a02:6b6:b060:6000::3:3c', 5432),  # man
    ])
    master = AsyncMock(return_value=('2a02:6b5:b050:5000::4:4c', 5432))  # sas

    os.environ['DEPLOY_NODE_DC'] = 'sas'
    pool = MdbSentinelConnectionPool(service_name='test', sentinel_manager=MockSentinel())
    with (
        patch.object(pool, 'get_slaves', side_effect=slaves),
        patch.object(pool.mdb_slaves, 'get_dc', side_effect=['vla', 'sas', 'man']),
    ):
        assert await get_one(pool.rotate_slaves()) == ('2a02:6b7:b070:7000::2:2c', 5432)

    os.environ['DEPLOY_NODE_DC'] = 'myt'
    pool = MdbSentinelConnectionPool(service_name='test', sentinel_manager=MockSentinel())
    with (
        patch.object(pool, 'get_slaves', side_effect=slaves),
        patch.object(pool.mdb_slaves, 'get_dc', side_effect=['vla', 'sas', 'man']),
    ):
        assert await get_one(pool.rotate_slaves()) == ('2a02:6b8:b080:8000::1:1c', 5432)

    pool = MdbSentinelConnectionPool(service_name='test', sentinel_manager=MockSentinel())
    with (
        patch.object(pool, 'get_slaves', side_effect=AsyncMock(return_value=None)),
        patch.object(pool, 'get_master_address', side_effect=master),
    ):
        assert await get_one(pool.rotate_slaves()) == ('2a02:6b5:b050:5000::4:4c', 5432)

    pool = MdbSentinelConnectionPool(service_name='test', sentinel_manager=MockSentinel())
    with (
        patch.object(pool, 'get_slaves', side_effect=AsyncMock(return_value=None)),
        patch.object(pool, 'get_master_address', side_effect=MasterNotFoundError),
    ):
        with pytest.raises(SlaveNotFoundError):
            await get_one(pool.rotate_slaves())


def test_mdb_sentinel__master_for():
    sentinel = MdbSentinel([('localhost', 26279)])
    redis = sentinel.master_for('test')
    assert isinstance(redis.connection_pool, MdbSentinelConnectionPool)


def test_mdb_sentinel__slave_for():
    sentinel = MdbSentinel([('localhost', 26279)])
    redis = sentinel.slave_for('test')
    assert isinstance(redis.connection_pool, MdbSentinelConnectionPool)
