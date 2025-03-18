# -*- coding: utf-8 -*-
import logging

from aioredis.sentinel import SentinelConnectionPool, Sentinel
from mdb_redis.sentinel import MdbSortedSlaves
from redis.sentinel import MasterNotFoundError, SlaveNotFoundError


logger = logging.getLogger(__name__)


class MdbSentinelConnectionPool(SentinelConnectionPool):
    def __init__(self, *args, **kwargs):
        self.mdb_slaves = MdbSortedSlaves()
        super().__init__(*args, **kwargs)

    async def get_slaves(self):
        return await self.sentinel_manager.discover_slaves(self.service_name)

    async def rotate_slaves(self):
        slaves = await self.get_slaves()
        if slaves:
            for slave in self.mdb_slaves.get_sorted_slaves(slaves):
                logger.debug('Redis: slave %r', slave)
                yield slave

        # Fallback to the master connection
        try:
            yield await self.get_master_address()
        except MasterNotFoundError:
            pass
        raise SlaveNotFoundError('Redis: no slave found for %s' % self.service_name)


class MdbSentinel(Sentinel):
    def master_for(self, *args, connection_pool_class=MdbSentinelConnectionPool, **kwargs):
        return super().master_for(*args, connection_pool_class=connection_pool_class, **kwargs)

    def slave_for(self, *args, connection_pool_class=MdbSentinelConnectionPool, **kwargs):
        return super().slave_for(*args, connection_pool_class=connection_pool_class, **kwargs)
