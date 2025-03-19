LIBRARY()

OWNER(
    sankear
    g:base
)

PEERDIR(
    library/cpp/offroad/custom
    library/cpp/offroad/tuple
    library/cpp/offroad/sub
    kernel/doom/offroad_common
    kernel/doom/wad
)

SRCS(
    offroad_inv_common.h
    offroad_inv_wad_io.h
    offroad_inv_wad_iterator.h
    offroad_inv_wad_reader.h
    offroad_inv_wad_sampler.h
    offroad_inv_wad_searcher.h
    offroad_inv_wad_writer.h
)

END()
