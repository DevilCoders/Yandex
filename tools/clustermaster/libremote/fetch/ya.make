LIBRARY()

OWNER(
    g:clustermaster
)

SRCS(
    sockhandler.cpp
)

PEERDIR(
    contrib/libs/matrixssl
    library/cpp/logger
)

END()
