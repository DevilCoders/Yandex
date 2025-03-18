LIBRARY()

OWNER(ivanmorozov)

SRCS(
    messenger.cpp
    messenger_impl.cpp
)

PEERDIR(
    library/cpp/balloc/optional
    library/cpp/logger/global
    library/cpp/deprecated/atomic
)

END()
