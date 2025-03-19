OWNER(
    g:blender
)

LIBRARY()

PEERDIR(
    library/cpp/scheme
)

SRCS(
    bigrams.cpp
    counters.sc
    factors.cpp
    typedefs.cpp
)

GENERATE_ENUM_SERIALIZATION(ui_enum.h)

END()
