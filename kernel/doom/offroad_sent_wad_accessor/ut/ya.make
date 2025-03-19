UNITTEST_FOR(kernel/doom/offroad_sent_wad_accessor)

OWNER(
    g:base
    rymis
)

PEERDIR(
    library/cpp/offroad/codec
    kernel/doom/wad
    library/cpp/offroad/tuple
    library/cpp/offroad/custom
    kernel/doom/hits
    library/cpp/offroad/streams
)

SRCS(
    offroad_sent_wad_accessor_ut.cpp
)

SIZE(MEDIUM)

END()
