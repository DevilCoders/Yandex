PY23_LIBRARY()

OWNER(velom)

PEERDIR(
    library/cpp/string_utils/tskv_format
)

PY_SRCS(
    __init__.py
    tskv.pyx
)

SRCS(
    tskv.cpp
)

END()

RECURSE_FOR_TESTS(
    tests
)
