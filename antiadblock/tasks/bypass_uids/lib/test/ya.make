PY3TEST()

OWNER(g:antiadblock)

TEST_SRCS(
    test_util.py
)

PEERDIR(
    contrib/python/numpy

    antiadblock/tasks/tools
    antiadblock/tasks/bypass_uids/lib
)

END()
