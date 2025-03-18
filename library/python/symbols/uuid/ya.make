LIBRARY()

OWNER(pg)

PEERDIR(
    contrib/libs/uuid
    library/python/symbols/registry
)

SRCS(
    GLOBAL syms.cpp
)

END()
