LIBRARY()

OWNER(ironpeter)

SRCS(
    comptable.cpp
)

PEERDIR(
    library/cpp/compproto
)

END()

RECURSE(
    usage
    ut
)
