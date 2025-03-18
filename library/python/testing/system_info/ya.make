PY2_LIBRARY()

OWNER(g:yatool anastasy888)

PY_SRCS(__init__.py)

PEERDIR(
    contrib/python/psutil
)

END()

RECURSE_FOR_TESTS(
    test
)
