PY23_LIBRARY()

OWNER(g:yql)

TEST_SRCS(test.py)

PEERDIR(
    library/python/protobuf/yql
    library/python/protobuf/yql/ut/protos
)

END()

RECURSE_FOR_TESTS(
    py2
    py3
)
