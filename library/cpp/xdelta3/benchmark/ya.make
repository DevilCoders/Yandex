G_BENCHMARK()

TAG(ya:fat)

OWNER(leasid)

SRCS(
    benchmark.cpp
)

PEERDIR(
    library/cpp/xdelta3/xdelta_codec
    library/cpp/xdelta3/ut/rand_data
)

SIZE(LARGE)

END()
