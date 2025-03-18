LIBRARY()

OWNER(
    ddoarn
    g:kikimr
)

SRCS(
    grpc_client_low.cpp
)

PEERDIR(
    contrib/libs/grpc
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
