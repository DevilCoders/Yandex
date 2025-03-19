LIBRARY()

OWNER(
    sankear
    g:base
)

SRCS(
    offroad_struct_diff_wad_iterator.h
    offroad_struct_diff_wad_reader.h
    offroad_struct_diff_wad_sampler.h
    offroad_struct_diff_wad_searcher.h
    offroad_struct_diff_wad_writer.h
    struct_diff_common.h
    struct_diff_input_buffer.h
    struct_diff_output_buffer.h
    struct_diff_reader.h
    struct_diff_seeker.h
)

PEERDIR(
    kernel/doom/wad
    library/cpp/pop_count
    library/cpp/offroad/codec
    library/cpp/offroad/custom
    library/cpp/offroad/flat
    library/cpp/offroad/keyinv
    library/cpp/offroad/tuple
    library/cpp/offroad/utility
)

END()
