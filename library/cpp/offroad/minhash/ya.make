LIBRARY()

OWNER(
    g:base
)

SRCS(
    writer.cpp
    common.cpp
)

PEERDIR(
    library/cpp/minhash
    library/cpp/offroad/flat
    library/cpp/offroad/tuple
)

END()
