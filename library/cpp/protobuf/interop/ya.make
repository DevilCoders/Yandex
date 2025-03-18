LIBRARY()

OWNER(
    paxakor
)

SRCS(
    cast.cpp
)

PEERDIR(
    contrib/libs/protobuf
)

END()

RECURSE_FOR_TESTS(
    ut
)
