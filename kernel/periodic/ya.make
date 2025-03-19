LIBRARY()

OWNER(
    ssmike
    g:base
)

SRCS(
    periodic.cpp
)

PEERDIR(
    kernel/searchlog
    library/cpp/threading/future
)

END()
