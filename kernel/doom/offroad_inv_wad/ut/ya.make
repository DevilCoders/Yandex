UNITTEST_FOR(kernel/doom/offroad_inv_wad)

OWNER(
    g:base
)

SRCS(
    offroad_inv_wad_io_ut.cpp
)

PEERDIR(
    library/cpp/digest/md5
    kernel/doom/offroad
    kernel/doom/wad
)

END()
