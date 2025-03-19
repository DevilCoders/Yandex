LIBRARY()

OWNER(
    g:base
)

SRCS(
    offroad_minhash_wad_searcher.cpp
    offroad_minhash_wad_writer.cpp
)

PEERDIR(
    kernel/doom/wad
    kernel/doom/offroad_key_wad
    library/cpp/offroad/flat
    library/cpp/offroad/minhash
)

END()
