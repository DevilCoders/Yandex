LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    wad_writer.h
    typed_wad_writer.h
    dummy.cpp
)

PEERDIR(
    library/cpp/offroad/flat
    library/cpp/offroad/custom
)

END()
