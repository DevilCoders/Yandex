LIBRARY()

OWNER(
    ermolovd
    g:messagebus
)

PEERDIR(
    library/cpp/messagebus
    library/cpp/messagebus/protobuf
)

SRCS(
    wrapper.cpp
)

END()
