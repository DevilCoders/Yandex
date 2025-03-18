PY23_LIBRARY()

OWNER(g:crypta)

PY_SRCS(
    proto2json.pyx
    json2proto.pyx
)

PEERDIR(
    contrib/libs/protobuf
    contrib/python/six
    library/cpp/protobuf/dynamic_prototype
    library/cpp/protobuf/json
    library/python/protobuf/dynamic_prototype
)

END()

RECURSE_FOR_TESTS(ut)
