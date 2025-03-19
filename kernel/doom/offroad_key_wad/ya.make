LIBRARY()

OWNER(
    elric
    g:base
)

PEERDIR(
    library/cpp/offroad/custom
    library/cpp/offroad/key
    kernel/doom/offroad_common
    kernel/doom/wad
    kernel/doom/wad
)

SRCS(
    combiners.h
    combined_key_data_reader.h
    offroad_key_wad_io.h
    offroad_key_wad_iterator.h
    offroad_key_wad_reader.h
    offroad_key_wad_sampler.h
    offroad_key_wad_searcher.h
    offroad_key_wad_writer.h
)

END()
