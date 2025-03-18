PY23_LIBRARY()

OWNER(g:tools-python)

PEERDIR(
    contrib/python/mock
    contrib/python/six
    library/python/ylock
)

TEST_SRCS(
    thread_tests.py
)

PY_SRCS(
    base.py
)

END()

RECURSE(
    py2
    py3
)
