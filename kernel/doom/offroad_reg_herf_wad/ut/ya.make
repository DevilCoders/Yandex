UNITTEST_FOR(kernel/doom/offroad_reg_herf_wad)

OWNER(
    g:base
)

SRCS(
    reg_erf_wad_ut.cpp
)

PEERDIR(
    library/cpp/digest/md5
    kernel/doom/algorithm
    kernel/doom/wad
    ysite/yandex/erf
    kernel/relevstatic
)

END()
