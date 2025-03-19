LIBRARY()

OWNER(
    elric
    kcd
    mvel
    g:base
    v01d
)

SRCS(
    decoding_index_reader.h
    encoding_index_writer.h
    extended_index_writer.h
    hit_filtering_index_reader.h
    hit_transforming_index_reader.h
    key_filtering_index_reader.h
    read_adaptors.h
    write_adaptors.h
    multi_source_index_reader.h
    multi_key_index_reader.h
    multi_key_index_writer.h
    sorting_index_writer.h
    unique_index_reader.h
    dummy.cpp
)

PEERDIR(
    kernel/search_types
    kernel/doom/hits
    kernel/doom/key
    kernel/doom/progress
    library/cpp/containers/mh_heap
)

END()
