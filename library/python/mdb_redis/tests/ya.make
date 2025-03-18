PY3TEST()

OWNER(g:python-contrib)

PEERDIR(
    contrib/python/pytest
    contrib/python/pytest-asyncio
    library/python/mdb_redis
)

TEST_SRCS(
    test_sentinel.py
    test_aiosentinel.py
)

END()
