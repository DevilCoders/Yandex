OWNER(g:cloud-dwh)

PY3TEST()

SIZE(SMALL)

PEERDIR(
    contrib/python/pytest-asyncio

    cloud/dwh/utils
)

TEST_SRCS(
    test_coroutines.py
    test_datetimeutils.py
    test_log.py
    test_misc.py
)

END()
