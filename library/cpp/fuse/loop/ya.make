LIBRARY()

OWNER(g:arc)

SRCS(
    counters.cpp
    processor.cpp
    threaded.cpp
)

IF (OS_LINUX)
    SRCS(
        epoll.cpp
    )
ENDIF()

PEERDIR(
    library/cpp/fuse
    library/cpp/logger
    library/cpp/monlib/dynamic_counters
)

END()
