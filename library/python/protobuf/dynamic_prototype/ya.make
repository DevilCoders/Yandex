PY23_LIBRARY()

OWNER(g:crypta)

PY_SRCS(
    dynamic_prototype.pyx
)

PEERDIR(
    contrib/libs/protobuf
    contrib/python/six
    library/cpp/protobuf/dynamic_prototype
    library/python/protobuf/get_serialized_file_descriptor_set
)

END()
