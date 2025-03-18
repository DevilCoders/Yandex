OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    empty.cpp
    fastlex_gen.h.rl
)

PEERDIR(
    contrib/libs/libxml
    library/cpp/charset
    library/cpp/html/entity
    library/cpp/logger
    library/cpp/packedtypes
)

END()
