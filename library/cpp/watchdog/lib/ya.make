LIBRARY()

OWNER(
    mvel
    g:base
)

SRCS(
    factory.cpp
    handle.cpp
    interface.cpp
)

PEERDIR(
    library/cpp/deprecated/atomic
)

END()
