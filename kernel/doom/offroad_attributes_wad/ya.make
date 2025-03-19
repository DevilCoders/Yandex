LIBRARY()

OWNER(
    g:base
)

SRCS(
    attributes_hit_count_calcer.h
    offroad_attributes_wad_io.h
    offroad_attributes_adaptors.h
)

PEERDIR(
    kernel/doom/hits
    kernel/doom/offroad
    kernel/doom/offroad_inv_wad
    kernel/doom/offroad_key_wad
    kernel/doom/offroad_keyinv_wad
    library/cpp/offroad/offset
)

END()

