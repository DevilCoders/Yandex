from functools import cached_property, wraps

from redis.sentinel import Sentinel
from redis import Redis
from pydantic import BaseSettings


class RedisSentinelSettings(BaseSettings):
    hosts: list[str] = []
    port: int = 26379
    cluster_name: str = ''
    password: str = ''
    socket_timeout: float = 0.1
    db: int = 0

    class Config:
        env_prefix = 'redis_'
        keep_untouched = (cached_property,)

    def make_connection_url(self) -> str:
        if self.hosts == ['localhost']:
            return f'redis://localhost:{self.port}/{self.db}'
        return ''.join(
            f'sentinel://:{self.password}@{host}:{self.port}/{self.db};'
            for host in self.hosts
        )

    @cached_property
    def sentinel(self) -> Sentinel:
        if self.hosts == ['localhost']:
            return PseudoSentinel(port=self.port)

        if not self.hosts:
            raise RuntimeError("Empty hosts, looks like you've forgot about environmental variable")

        return Sentinel(
            [(h, self.port) for h in self.hosts],
            socket_timeout=self.socket_timeout,
        )

    def get_redis_master(self) -> Redis:
        return self.sentinel.master_for(
            self.cluster_name,
            password=self.password,
            db=self.db,
        )

    def get_redis_slave(self) -> Redis:
        return self.sentinel.slave_for(
            self.cluster_name,
            password=self.password,
            db=self.db,
        )

    def make_celery_broker_transport_options(self) -> dict[str, str]:
        return {'master_name': self.cluster_name}


class PseudoSentinel(Sentinel):
    def __init__(self, port: int):
        self.redis = Redis(port=port)

    def master_for(self, service_name, **kwargs):
        return self.redis

    def slave_for(self, service_name, **kwargs):
        return self.redis


def redis_lock(redis: Redis, timeout=3600):
    def _redis_lock(f):
        lock_key_name = f'LOCK_{f.__module__}:{f.__name__}'
        lock = redis.lock(
            lock_key_name,
            timeout=timeout,
        )

        @wraps(f)
        def wrapper(*args, **kwargs):
            if lock.acquire(blocking=False):
                try:
                    f(*args, **kwargs)
                finally:
                    lock.release()

        return wrapper
    return _redis_lock
