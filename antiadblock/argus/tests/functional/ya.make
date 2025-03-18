OWNER(g:antiadblock)

PY3TEST()

TEST_SRCS(
    conftest.py
    test_browser_pool.py
)

PEERDIR(
    contrib/python/pytest
    contrib/python/mock

    antiadblock/argus/bin
)

REQUIREMENTS(network:full)

SIZE(MEDIUM)

END()
