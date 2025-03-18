PY23_LIBRARY()

OWNER(
    g:contrib
)

TEST_SRCS(test.py)

PY_SRCS(test.proto)

PEERDIR(
    library/python/protobuf/argparse
)

END()

RECURSE_FOR_TESTS(
    py2
    py3
)
