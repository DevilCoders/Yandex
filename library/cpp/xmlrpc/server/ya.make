LIBRARY()

OWNER(pg)

PEERDIR(
    library/cpp/neh
    library/cpp/xmlrpc/protocol
)

SRCS(
    server.cpp
)

RUN_LUA(
    gen.lua 10
    STDOUT genwrap.h
)

END()
