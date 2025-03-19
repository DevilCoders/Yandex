LIBRARY()

OWNER(
    sankear
    g:base
)

SRCS(
    doc_data_holder.h
    offroad_struct_wad_io.h
    offroad_struct_wad_iterator.h
    offroad_struct_wad_reader.h
    offroad_struct_wad_sampler.h
    offroad_struct_wad_searcher.h
    offroad_struct_wad_writer.h
    serialized_struct_reader.h
    struct_reader.h
    struct_type.h
)

PEERDIR(
    kernel/doom/chunked_wad
    kernel/doom/search_fetcher
    library/cpp/offroad/byte_stream
    library/cpp/offroad/custom
    library/cpp/offroad/keyinv
)

END()
