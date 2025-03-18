PY23_LIBRARY()

OWNER(
    g:crypta
)

PEERDIR(
    contrib/python/protobuf
)

PY_SRCS(__init__.py)

END()

RECURSE_FOR_TESTS(
    ut
)