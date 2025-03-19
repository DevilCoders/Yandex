LIBRARY()

OWNER(
    g:base
    mvel
)

SRCS(
    cntintrf.cpp
    lib.cpp
)

PEERDIR(
    kernel/search_types
)

GENERATE_ENUM_SERIALIZATION(reqtypes.h)

GENERATE_ENUM_SERIALIZATION(basetype.h)

END()
