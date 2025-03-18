LIBRARY()

OWNER(
    kostik
    bulatman
)

PEERDIR(
    contrib/libs/protobuf
)

SRCS(
    packer.cpp
    syncword.cpp
    unpacker.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(
    benchmark
    ut
)
