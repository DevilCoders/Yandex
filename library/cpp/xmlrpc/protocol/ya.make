LIBRARY()

OWNER(pg)

PEERDIR(
    library/cpp/string_utils/base64
    library/cpp/blockcodecs
    library/cpp/xml/document
    library/cpp/xml/encode
)

SRCS(
    value.cpp
    protocol.cpp
    compress.cpp
    rpcfault.cpp
)

END()
