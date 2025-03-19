LIBRARY()

OWNER(
    g:base
)

SRCS(
    offroad_block_string_wad_io.h
    offroad_block_string_wad_reader.h
    offroad_block_string_wad_sampler.h
    offroad_block_string_wad_searcher.h
    offroad_block_string_wad_writer.h
)

PEERDIR(
    kernel/doom/wad
    library/cpp/offroad/custom
    library/cpp/offroad/key
)

END()
