import asyncpgsa

from pyscopg2.asyncpg import PoolManager as AsyncPgPoolManager
from pyscopg2.utils import Dsn


class PoolManager(AsyncPgPoolManager):
    async def _pool_factory(self, dsn: Dsn):
        return await asyncpgsa.create_pool(
            str(dsn), **self.pool_factory_kwargs,
        )


__all__ = ["PoolManager"]
