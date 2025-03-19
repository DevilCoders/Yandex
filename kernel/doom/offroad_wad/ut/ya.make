UNITTEST_FOR(kernel/doom/offroad_wad)

OWNER(
    elric
    g:base
)

SRCS(
    offroad_wad_io_ut.cpp
    offroad_wad_key_buffer_ut.cpp
)

PEERDIR(
    library/cpp/digest/md5
    kernel/doom/algorithm
    kernel/doom/simple_map
    kernel/doom/wad
)

END()
