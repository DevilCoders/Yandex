LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    key_data_traits.h
    standard_index_reader.h
    standard_index_searcher.h
    standard_index_writer.h
    dummy.cpp
)

PEERDIR(
    library/cpp/offroad/key
    library/cpp/offroad/keyinv
    library/cpp/offroad/sub
    library/cpp/offroad/tuple
)

END()
