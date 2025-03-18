LIBRARY()

OWNER(pg)

PEERDIR(
    library/cpp/coroutine/engine
    library/cpp/coroutine/listener
    library/cpp/http/io
    library/cpp/deprecated/atomic
)

SRCS(
    server.cpp
)

END()
