LIBRARY()

OWNER(
    kostik
    g:orange
)

PEERDIR(
    contrib/libs/protobuf
    library/cpp/logger/global
)

SRCS(
    protofile.cpp
)

END()
