OWNER(
    g:yatest
    torkve
)

PY3_LIBRARY()

PEERDIR(
    contrib/python/mypy
)

TEST_SRCS(types_test.py)

END()
