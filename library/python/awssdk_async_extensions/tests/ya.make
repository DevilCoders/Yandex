PY3TEST()

OWNER(
    g:s3
    paxakor
)

PEERDIR(
    contrib/python/pytest-asyncio
    library/python/awssdk_async_extensions/lib/core
)

TEST_SRCS(
    test_all.py
)

SIZE(SMALL)

END()

