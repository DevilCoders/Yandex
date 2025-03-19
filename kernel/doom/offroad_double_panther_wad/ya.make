LIBRARY()

OWNER(
    sankear
    g:base
)

SRCS(
    offroad_double_panther_wad_io.h
    double_panther_hit_range.h
    double_panther_hit_range_adaptors.h
)

PEERDIR(
    kernel/doom/offroad
    kernel/doom/offroad_inv_wad
    kernel/doom/offroad_key_wad
    kernel/doom/offroad_keyinv_wad
)

END()
