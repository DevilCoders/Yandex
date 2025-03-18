PY23_LIBRARY()

OWNER(
    g:contrib
)

PY_SRCS(__init__.py)

PEERDIR(
    contrib/python/PyYAML
    library/cpp/getoptpb/proto
)

END()

RECURSE_FOR_TESTS(
    ut
)
