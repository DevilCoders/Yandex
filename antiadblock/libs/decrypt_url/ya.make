LIBRARY()

OWNER(g:antiadblock)

SRCS(
    lib/cry.cpp
)

PY_SRCS(
    lib/__init__.py
    lib/cry.pyx
)

PEERDIR(
    contrib/libs/openssl
    contrib/libs/re2
    library/cpp/string_utils/base64
    library/cpp/uri
)

END()

RECURSE_FOR_TESTS (
    tests
)
