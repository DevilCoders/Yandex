LIBRARY()

OWNER(nsofya)

SRCS(
    GLOBAL compressor.cpp
)

PEERDIR(
    kernel/multipart_archive/abstract
    kernel/multipart_archive/config
    library/cpp/streams/lz
    library/cpp/streams/lzma
    library/cpp/logger/global
)

END()
