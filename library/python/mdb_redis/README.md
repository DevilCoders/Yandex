Выбираем реплику редиса по гео.

Пример использования

```python
DJANGO_REDIS_CONNECTION_FACTORY = 'django_redis.pool.SentinelConnectionFactory'

REDIS_CLUSTER = 'mytest_cluster'
REDIS_DB = 1
REDIS_PASSWORD = 'secr3t'
REDIS_HOSTS = [
    ('sas-1.db.yandex.net', 26379),
    ('sas-2.db.yandex.net', 26379),
    ('vla-3.db.yandex.net', 26379),
    ('vla-4.db.yandex.net', 26379),
]

CACHES = {
    'default': {
        'BACKEND': 'django_redis.cache.RedisCache',
        'LOCATION': f'redis://{REDIS_CLUSTER}/{REDIS_DB}',
        'KEY_PREFIX': 'test',
        'TIMEOUT': 300,  # sec
        'OPTIONS': {
            'PASSWORD': REDIS_PASSWORD,
            'CLIENT_CLASS': 'django_redis.client.SentinelClient',
            'SENTINELS': REDIS_HOSTS,
            'SENTINEL_KWARGS': {},
            'CONNECTION_POOL_CLASS': 'mdb_redis.MdbSentinelConnectionPool',
        },
    }
}
```
