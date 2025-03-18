LIBRARY()

OWNER(
    elric
    sankear
    g:base
)

SRCS(
    flat_item.h
    flat_searcher.h
    flat_ui32_searcher.h
    flat_ui64_searcher.h
    flat_writer.h
    flat_common.cpp
)

PEERDIR(
    library/cpp/offroad/offset
    library/cpp/offroad/streams
    library/cpp/offroad/utility
)

END()
