UNITTEST_FOR(library/cpp/http/io)

OWNER(g:util)

PEERDIR(
    library/cpp/http/server
)

SRCS(
    chunk_ut.cpp
    compression_ut.cpp
    headers_ut.cpp
    stream_ut.cpp
    stream_ut_medium.cpp
)

END()
