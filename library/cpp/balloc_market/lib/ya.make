LIBRARY()

OWNER(
    omakovski
)

NO_UTIL()

NO_COMPILER_WARNINGS()

SRCS(
    alloc_stats.cpp
    alloc_stats.h
)

IF (OS_LINUX)
    PEERDIR(
        contrib/libs/linuxvdso
        contrib/libs/numa
    )
ENDIF()

PEERDIR(
    library/cpp/balloc_market/setup
    library/cpp/malloc/api
)

SET(IDE_FOLDER "util")

END()
