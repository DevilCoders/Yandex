LIBRARY()

OWNER(
    elric
    sankear
    g:base
)

SRCS(
    sub_writer.h
    sub_reader.h
    sub_sampler.h
    sub_seeker.h
    dummy.cpp
)

PEERDIR(
    library/cpp/offroad/offset
    library/cpp/offroad/utility
    library/cpp/offroad/flat
)

END()
