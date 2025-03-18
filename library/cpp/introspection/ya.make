LIBRARY()

OWNER(
    bulatman
    g:util
    emazhukin
)

SRCS(
    hash_ops.cpp
    introspection.cpp
)

PEERDIR(
    contrib/libs/pfr
)

END()

RECURSE_FOR_TESTS(tests)
