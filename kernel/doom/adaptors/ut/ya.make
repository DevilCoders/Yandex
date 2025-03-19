UNITTEST_FOR(kernel/doom/adaptors)

OWNER(
    elric
    g:base
    sankear
    v01d
)

SRCS(
    multi_key_index_writer_ut.cpp
    multi_source_index_reader_ut.cpp
    transforming_key_writer_ut.cpp
)

PEERDIR(
    kernel/doom/hits
    kernel/doom/key
    kernel/doom/simple_map
)

END()
