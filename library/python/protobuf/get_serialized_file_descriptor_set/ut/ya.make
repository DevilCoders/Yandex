PY23_LIBRARY()

OWNER(g:crypta)

TEST_SRCS(
    test.py
)

PY_SRCS(
    my_inner_message.proto
    my_message.proto
)

PEERDIR(
    library/python/protobuf/get_serialized_file_descriptor_set
)

END()

RECURSE_FOR_TESTS(
    py2
    py3
)