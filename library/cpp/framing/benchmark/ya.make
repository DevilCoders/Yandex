G_BENCHMARK()

OWNER(kostik bulatman)

SRCS(
    test.proto

    packer_bench.cpp
    unpacker_bench.cpp
)

PEERDIR(
    library/cpp/framing
)

END()
