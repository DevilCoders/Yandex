LIBRARY()

OWNER(
    omakovski
)

NO_UTIL()

IF (OS_WINDOWS)
    PEERDIR(
        library/cpp/lfalloc
    )
ELSE()
    SRCS(
        balloc.cpp
        malloc-info.cpp
    )
    PEERDIR(
        library/cpp/balloc_market/lib
    )
ENDIF()

END()
