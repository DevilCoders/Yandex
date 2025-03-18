PY3_LIBRARY()

OWNER(g:python-contrib)

VERSION(0.0.1)

PEERDIR(
    contrib/python/redis
    contrib/python/aioredis/aioredis-2
)

PY_SRCS(
    TOP_LEVEL
    mdb_redis/__init__.py
    mdb_redis/sentinel.py
    mdb_redis/aiosentinel.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
