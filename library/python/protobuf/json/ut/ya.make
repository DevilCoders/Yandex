PY23_TEST()

OWNER(g:crypta)

TEST_SRCS(
    test.py
)

PY_SRCS(
    my_inner_message.proto
    my_message.proto
)

PEERDIR(
    library/python/protobuf/json
)

END()
