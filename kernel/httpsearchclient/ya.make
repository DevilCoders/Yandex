LIBRARY()

OWNER(
    kulikov
    vmordovin
    g:middle
)

SRCS(
    conniterator.cpp
    httpsearchclient.cpp
)

PEERDIR(
    library/cpp/uri
    library/cpp/dns
    kernel/httpsearchclient/config
    kernel/index_generation
    library/cpp/deprecated/atomic
)

END()
