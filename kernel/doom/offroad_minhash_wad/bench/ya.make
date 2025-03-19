Y_BENCHMARK()

OWNER(g:base)

SRCS(
    bench.cpp
)

PEERDIR(
    kernel/doom/item_storage
    kernel/doom/offroad_key_wad
    kernel/doom/offroad_minhash_wad

    library/cpp/offroad/custom
    library/cpp/offroad/minhash
    library/cpp/testing/benchmark
)

END()
