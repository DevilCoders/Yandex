LIBRARY()

OWNER(orivej)

PEERDIR(
    contrib/libs/openssl
    library/python/symbols/registry
)

SRCS(
    GLOBAL syms.cpp
)

END()
