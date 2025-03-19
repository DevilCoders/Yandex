LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    attributes_hit_adaptors.h
    counts_hit_adaptors.h
    panther_hit_adaptors.h
    offroad_io_factory.cpp
    index_tables.h
    offroad_reader.h
    offroad_writer.h
    offroad_searcher.h
    offroad_io.h
    offroad_panther_io.h
    offroad_counts_io.h
)

PEERDIR(
    kernel/doom/hits
    kernel/doom/info
    kernel/doom/standard_models
    library/cpp/offroad/keyinv
    library/cpp/offroad/standard
    library/cpp/offroad/utility
)

END()
