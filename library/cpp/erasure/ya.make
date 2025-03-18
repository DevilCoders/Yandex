LIBRARY()

OWNER(
    danlark
    ignat
    akozhikhov
    g:yt
    g:base
)

SRCS(
    public.cpp
    codec.cpp
    helpers.cpp

    isa_erasure.cpp
    jerasure.cpp

    reed_solomon.cpp
    reed_solomon_isa.cpp
    reed_solomon_jerasure.cpp

    lrc.cpp
    lrc_isa.cpp
    lrc_jerasure.cpp
)

PEERDIR(
    contrib/libs/isa-l/erasure_code
    contrib/libs/jerasure
    library/cpp/sse
    library/cpp/yt/assert
)

GENERATE_ENUM_SERIALIZATION(public.h)

END()
