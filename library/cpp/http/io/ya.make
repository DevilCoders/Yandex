LIBRARY()

OWNER(
    g:util
    mvel
)

PEERDIR(
    library/cpp/blockcodecs
    library/cpp/streams/brotli
    library/cpp/streams/bzip2
    library/cpp/streams/lzma
)

SRCS(
    chunk.cpp
    compression.cpp
    headers.cpp
    stream.cpp
)

END()

RECURSE(
    fuzz
    list_codings
    ut
)
