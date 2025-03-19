LIBRARY()

OWNER(
    elric
    kcd
    g:base
)

SRCS(
    dummy.cpp
    simple_map_reader.h
    simple_map_writer.h
)

PEERDIR(
    kernel/doom/hits
    kernel/doom/progress
)

END()
