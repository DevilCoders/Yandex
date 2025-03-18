PY23_LIBRARY()

OWNER(velom)

TEST_SRCS(test_tskv.py)

PEERDIR(
    library/python/tskv
)

END()

RECURSE(
    py2
    py3
)
