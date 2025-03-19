LIBRARY()

OWNER(g:kwyt)

PEERDIR(
    library/cpp/logger
)

SRCS(
    debug_guard.cpp
    flushing_stream_backend.cpp
    log.cpp
)

END()
