LIBRARY()

OWNER(
    elric
    g:base
)

PEERDIR(
    kernel/doom/info
    kernel/doom/offroad_ann_data_wad
    kernel/doom/chunked_wad
    library/cpp/offroad/utility
    library/cpp/offroad/wad
)

SRCS(
    offroad_ann_data_wad_accessor.cpp
)

END()
