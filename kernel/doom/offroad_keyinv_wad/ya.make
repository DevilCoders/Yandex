LIBRARY()

OWNER(
    sankear
    g:base
)

SRCS(
    default_hit_range.h
    default_hit_range_adaptors.h
    default_hit_range_accessor.h
    generic_hit_layers_range.h
    generic_hit_layers_range_accessor.h
    key_data_factory.h
    offroad_keyinv_wad_io.h
    offroad_keyinv_wad_reader.h
    offroad_keyinv_wad_sampler.h
    offroad_keyinv_wad_writer.h
)

PEERDIR(
    kernel/doom/progress
    kernel/doom/wad
)

END()
