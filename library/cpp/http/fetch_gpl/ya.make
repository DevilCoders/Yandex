LIBRARY()

OWNER(
    g:zora
)

PEERDIR(
    contrib/libs/matrixssl
    library/cpp/http/fetch
)

SRCS(
    sockhandler.cpp
    httpagent.h
    sockhandler.h
)

END()
